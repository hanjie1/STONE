/*************************************************************************
 *
 *  fadc_vxs_list.c - Library of routines for the user to write for
 *                    readout and buffering of events from JLab FADC using
 *                    a JLab pipeline TI module and Linux VME controller.
 *
 *                In this example, clock, syncreset, and trigger are
 *                output from the TI then distributed using a
 *                switch slot SD module
 *
 */

/* Event Buffer definitions */
#define MAX_EVENT_POOL     10
#define MAX_EVENT_LENGTH   1024*500      /* Size in Bytes */

/* Define Interrupt source and address */
#define TI_MASTER
#define TI_READOUT TI_READOUT_EXT_POLL  /* Poll for available data, external triggers */
#define TI_ADDR    (21<<19)          /* GEO slot 21 */

#define FIBER_LATENCY_OFFSET 0x4A  /* measured longest fiber length */

#include "dmaBankTools.h"
#include "tiprimary_list.c" /* source required for CODA */
#include "sdLib.h"
#include "fadcLib.h"        /* library of FADC250 routines */
#include "fadc250Config.h"
#include "HelBoard.c"
#include "timebrdLib.c"

#define BUFFERLEVEL 20
#define BLOCKLEVEL 20
#define TIRANDOMPULSER  // using TI internal random pulser as trigger
//#define TIFIXEDPULSER  // using TI internal fixed pulser as trigger
#define FADCPLAYBACK   // turn on fadc playback feature
#define USE_VTP
//#define USE_HELBOARD
//#define USE_HAPTB

int haptb_ramp_value = 12000;

/* FADC Library Variables */
extern int fadcA32Base, nfadc;
#define NFADC     1
/* Address of first fADC250 */
#define FADC_ADDR (3<<19)
/* Increment address to find next fADC250 */
#define FADC_INCR (1<<19)
#define FADC_BANK 0x3

#define FADC_READ_CONF_FILE {			\
    fadc250Config("");				\
    if(rol->usrConfig)				\
      fadc250Config(rol->usrConfig);		\
  }

/* for the calculation of maximum data words in the block transfer */
unsigned int MAXFADCWORDS=0;

/* SD variables */
static unsigned int sdScanMask = 0;

/* trigger counts to renew the tiSoftTrig */
unsigned int ntrigger=0; 

#define nsamples 32*16
unsigned short sdata[5][nsamples];  // five clusters 

/* function prototype */
void rocTrigger(int arg);

void
rocDownload()
{
  unsigned int iflag;
  int ifa, stat;

  /*****************
   *   TI SETUP
   *****************/

  /*
   * Set Trigger source
   *    For the TI-Master, valid sources:
   *      TI_TRIGGER_FPTRG     2  Front Panel "TRG" Input
   *      TI_TRIGGER_TSINPUTS  3  Front Panel "TS" Inputs
   *      TI_TRIGGER_TSREV2    4  Ribbon cable from Legacy TS module
   *      TI_TRIGGER_PULSER    5  TI Internal Pulser (Fixed rate and/or random)
   */
//#if defined(TIRANDOMPULSER) || defined(TIFIXEDPULSER)
//  tiSetTriggerSource( TI_TRIGGER_PULSER); /* TS Inputs enabled */
//#endif

#ifdef FADCPLAYBACK
  tiSetTriggerSourceMask(TI_TRIGSRC_TSINPUTS | TI_TRIGSRC_PULSER | TI_TRIGSRC_VME | TI_TRIGSRC_LOOPBACK);
#else
  tiSetTriggerSource(TI_TRIGGER_TSINPUTS);
#endif


  /* Enable set specific TS input bits (1-6) */
  tiEnableTSInput( TI_TSINPUT_1 );

  /* Load the trigger table that associates
   *    - TS#1,2,3,4,5,6 : Physics trigger,
   */
  tiLoadTriggerTable(0);

  tiSetTriggerHoldoff(1,10,0);
  tiSetTriggerHoldoff(2,10,0);

  /* Set the SyncReset width to 4 microSeconds */
  tiSetSyncResetType(1);

  /* Set initial number of events per block */
  blockLevel = BLOCKLEVEL;
  tiSetBlockLevel(blockLevel);

  /* Set Trigger Buffer Level */
  tiSetBlockBufferLevel(BUFFERLEVEL);

  /* Sync event every 1000 blocks */
  tiSetSyncEventInterval(0);

  /* Set L1A prescale ... rate/(x+1) */
  tiSetPrescale(0);

  /* Set TS input #1 prescale rate/(2^(x-1) + 1)*/
  tiSetInputPrescale(1, 0);

  /* Add trigger latch pattern to datastream */
  tiSetFPInputReadout(1);

#if defined(TIRANDOMPULSER) && defined(FADCPLAYBACK)
    /* Enable Random at rate 500kHz/(2^n) -- tiSetRandomTrigger(2,n)*/
    tiSetRandomTrigger(2,0xa);  // playback trigger
#elif defined(TIRANDOMPULSER) && (!defined(FADCPLAYBACK))
    tiSetRandomTrigger(1,0x8);
#endif

  /* Init the SD library so we can get status info */
  sdScanMask = 0;
  stat = sdInit(0);
  if (stat != OK)
    {
      printf("%s: WARNING: sdInit() returned %d\n",__func__, stat);
      daLogMsg("ERROR","SD not found");
    }

  /*****************
   *   FADC SETUP
   *****************/

  /* FADC Initialization flags */
  iflag = 0; /* NO SDC */
  iflag |= (1<<0);  /* VXS sync-reset */
  iflag |= FA_INIT_VXS_TRIG;  /* VXS trigger source */
  iflag |= FA_INIT_VXS_CLKSRC;  /* VXS 250MHz Clock source */
  iflag |= FA_INIT_SKIP_FIRMWARE_CHECK;  /* ignore firmware check */

  fadcA32Base = 0x09000000;
  printf("fadc iflag: %x\n",iflag);
  vmeSetQuietFlag(1);
  faInit(FADC_ADDR, FADC_INCR, NFADC, iflag);
  vmeSetQuietFlag(0);

#ifdef FADCPLAYBACK
  /* Set FADC Playback */
  for(ifa=0; ifa<nfadc; ifa++){
     int nchan,nsam;

     for(nchan=0; nchan<16; nchan++){ // chan 0-15 initial pedestals
        for(nsam=0;nsam<32;nsam++){
          sdata[0][nchan*32+nsam] = 100;
          sdata[1][nchan*32+nsam] = 100;
          sdata[2][nchan*32+nsam] = 100;
          sdata[3][nchan*32+nsam] = 100;
          sdata[4][nchan*32+nsam] = 100;
        }
     }

     for(nsam=0;nsam<32;nsam++){   // cluster 0
        if(nsam<5) sdata[0][nsam] = 225*nsam+100;
        if(nsam>=5 && nsam<15) sdata[0][nsam] = 1000-90*(nsam-4);

        if(nsam<6) sdata[0][nsam+32*5] = 100*nsam+100;
        if(nsam>=6 && nsam<11) sdata[0][nsam+32*5] = 600-100*(nsam-5);

        if(nsam<7) sdata[0][nsam+32*6] = 50*nsam+100;
        if(nsam>=7 && nsam<13) sdata[0][nsam+32*6] = 400-50*(nsam-6);
     }

     for(nsam=0;nsam<32;nsam++){   // cluster 1
        if(nsam<4) sdata[1][nsam+32*9] = 300*nsam+100;
        if(nsam>=4 && nsam<13) sdata[1][nsam+32*9] = 1000-100*(nsam-3);

        if(nsam<6) sdata[1][nsam+32*14] = 100*nsam+100;
        if(nsam>=6 && nsam<11) sdata[1][nsam+32*14] = 600-100*(nsam-5);

        if(nsam<7) sdata[1][nsam+32*15] = 50*nsam+100;
        if(nsam>=7 && nsam<13) sdata[1][nsam+32*15] = 400-50*(nsam-6);
     }

     for(nsam=0;nsam<32;nsam++){   // cluster 2
        if(nsam<6) sdata[2][nsam+32*7] = 180*nsam+100;
        if(nsam>=6 && nsam<15) sdata[2][nsam+32*7] = 1000-100*(nsam-5);

        if(nsam<6) sdata[2][nsam+32*2] = 160*nsam+100;
        if(nsam>=6 && nsam<11) sdata[2][nsam+32*2] = 900-160*(nsam-5);

        if(nsam<7) sdata[2][nsam+32*12] = 100*nsam+100;
        if(nsam>=7 && nsam<15) sdata[2][nsam+32*12] = 700-75*(nsam-6);

        if(nsam<6) sdata[2][nsam+32*13] = 100*nsam+100;
        if(nsam>=6 && nsam<11) sdata[2][nsam+32*13] = 600-100*(nsam-5);
     }

     for(nsam=0;nsam<32;nsam++){   // cluster 3
        if(nsam>=20 && nsam<26) sdata[3][nsam+32*1] = 80*(nsam-20)+100;
        if(nsam>=26 && nsam<31) sdata[3][nsam+32*1] = 500-80*(nsam-25);

        if(nsam>=2 && nsam<=6) sdata[3][nsam+32*2] = 100*(nsam-2)+100;
        if(nsam>6 && nsam<=10) sdata[3][nsam+32*2] = 500-100*(nsam-6);

        if(nsam>=1 && nsam<=6) sdata[3][nsam+32*3] = 100*(nsam-1)+100;
        if(nsam>6 && nsam<=11) sdata[3][nsam+32*3] = 600-100*(nsam-6);

        if(nsam>=1 && nsam<=7) sdata[3][nsam+32*8] = 150*(nsam-1)+100;
        if(nsam>7 && nsam<=13) sdata[3][nsam+32*8] = 1000-150*(nsam-7);

        if(nsam>=2 && nsam<=7) sdata[3][nsam+32*13] = 100*(nsam-2)+100;
        if(nsam>7 && nsam<=12) sdata[3][nsam+32*13] = 600-100*(nsam-7);
     }

     for(nsam=0;nsam<32;nsam++){   // cluster 4
        if(nsam>=2 && nsam<=6) sdata[4][nsam+32*1] = 100*(nsam-2)+100;
        if(nsam>6 && nsam<=10) sdata[4][nsam+32*1] = 500-100*(nsam-6);

        if(nsam>=1 && nsam<=4) sdata[4][nsam+32*2] = 200*(nsam-1)+100;
        if(nsam>4 && nsam<=10) sdata[4][nsam+32*2] = 700-100*(nsam-4);

        if(nsam>=1 && nsam<=7) sdata[4][nsam+32*6] = 150*(nsam-1)+100;
        if(nsam>7 && nsam<=16) sdata[4][nsam+32*6] = 1000-100*(nsam-7);

        if(nsam>=1 && nsam<=6) sdata[4][nsam+32*7] = 400*(nsam-1)+100;
        if(nsam>6 && nsam<=16) sdata[4][nsam+32*7] = 2100-200*(nsam-6);

        if(nsam>=3 && nsam<=10) sdata[4][nsam+32*8] = 100*(nsam-3)+100;
        if(nsam>10 && nsam<=17) sdata[4][nsam+32*8] = 800-100*(nsam-10);

        if(nsam>=2 && nsam<=6) sdata[4][nsam+32*12] = 200*(nsam-2)+100;
        if(nsam>6 && nsam<=14) sdata[4][nsam+32*12] = 900-100*(nsam-6);

        if(nsam>=2 && nsam<=6) sdata[4][nsam+32*13] = 100*(nsam-2)+100;
        if(nsam>6 && nsam<=10) sdata[4][nsam+32*13] = 500-100*(nsam-6);
     }

     faPPGDisable(faSlot(ifa));

     int PPG_status = faSetPPG(faSlot(ifa),0 , sdata[0], nsamples);
     if(PPG_status<0) printf("faSetPPG failed\n");
     else printf("faSetPPG initialize successfull\n");

   }

#endif

  /* Just one FADC250 */
  if(nfadc == 1)
    faDisableMultiBlock();
  else
    faEnableMultiBlock(1);

  /* configure all modules based on config file */
  FADC_READ_CONF_FILE;

  for(ifa = 0; ifa < nfadc; ifa++)
    {
      /* Bus errors to terminate block transfers (preferred) */
      faEnableBusError(faSlot(ifa));

      /*trigger-related*/
      faResetMGT(faSlot(ifa),1);
      faSetTrigOut(faSlot(ifa), 7);

      /* Enable busy output when too many events are being processed */
      faSetTriggerBusyCondition(faSlot(ifa), 3);

      /* set fadc scaler */
	
      int scaler_status = faSetScalerBlockInterval(faSlot(ifa),1);
      if(scaler_status<0) printf("faSetScalerBlockInterval failed\n");
      else printf("faSetScalerBlockInterval successfull\n");
    }

  sdSetActiveVmeSlots(faScanMask()); /* Tell the sd where to find the fadcs */

  /*****************
   *   VTP SETUP
   *****************/
#ifdef USE_VTP
  tiRocEnable(2);
/* BR: Added TI_BUSY_LOOPBACK and TI_BUSY_SWB here for testing - seems like it may be missing before or after unless this is done */
  tiSetBusySource(TI_BUSY_LOOPBACK | TI_BUSY_SWB | TI_BUSY_SWA, 0);
#endif

   // **  helicity board  **//
#ifdef USE_HELBOARD
  WriteHelBoard(0,5); //t_settle 60 us
  int hel_status1 = WriteHelBoard(1,27); // MPS 30 Hz
  //int hel_status1 = WriteHelBoard(1,23);   // MPS 120 Hz
  WriteHelBoard(2,0);
  WriteHelBoard(3,1); // quad 
  int hel_status4 = WriteHelBoard(4,3);
 

  if(hel_status1==0 && hel_status4==0) printf("Helicity board write successfully\n");
  else printf("Helicity board write failed\n");
#endif

   // **  happex timing board  **//
#ifdef USE_HAPTB
//30 Hz t_settle 60 us
  timebrdInit(0x2f20,0);
  timebrdDumpReg();
  timebrdSetDAC(2,34098);
  //timebrdGetDAC(2);
  printf("Happex timing board 1: GetDAC=%d\n",timebrdGetDAC(2));

  timebrdInit(0x2f30,0);
  timebrdDumpReg();
  timebrdSetDAC(2,34080);
  //timebrdGetDAC(2);
  printf("Happex timing board 2: GetDAC=%d\n",timebrdGetDAC(2));
#endif

  tiStatus(0);
  sdStatus(0);
  faGStatus(0);

  printf("rocDownload: User Download Executed\n");
  printf("Last compiled date/time: %s %s\n",__DATE__, __TIME__);

}

void
rocPrestart()
{
  int ifa;

  /* Program/Init VME Modules Here */
  for(ifa=0; ifa < nfadc; ifa++)
    {
      faSoftReset(faSlot(ifa),0);
      faResetToken(faSlot(ifa));
      faResetTriggerCount(faSlot(ifa));
      faEnableSyncReset(faSlot(ifa));
       // enable playback
#ifdef FADCPLAYBACK
      faPPGEnable(faSlot(ifa));
#endif
      faEnableScalers(faSlot(ifa));
    }

  /* Set number of events per block (broadcasted to all connected TI Slaves)*/
  tiSetBlockLevel(blockLevel);
  printf("rocPrestart: Block Level set to %d\n",blockLevel);

  tiSetEvTypeScalers(1);
  tiStatus(0);
  faGStatus(0);

  printf("rocPrestart: User Prestart Executed\n");

}

void
rocGo()
{
  int fadc_mode = 0, pl=0, ptw=0, nsb=0, nsa=0, np=0;
  /* Get the current block level */
  blockLevel = tiGetCurrentBlockLevel();
  printf("%s: Current Block Level = %d\n",
	 __FUNCTION__,blockLevel);

  faGSetBlockLevel(blockLevel);

  /* Get the FADC mode and window size to determine max data size */
  faGetProcMode(faSlot(0), &fadc_mode, &pl, &ptw,
		&nsb, &nsa, &np);

  /* Set Max words from fadc (proc mode == 1 produces the most)
     nfadc * ( Block Header + Trailer + 2  # 2 possible filler words
               blockLevel * ( Event Header + Header2 + Timestamp1 + Timestamp2 +
	                      nchan * (Channel Header + (WindowSize / 2) )
             ) +
     scaler readout # 16 channels + header/trailer
   */
  MAXFADCWORDS = nfadc * (4 + blockLevel * (4 + 16 * (1 + (ptw / 2))) + 18);

#if defined(TIFIXEDPULSER) && defined(FADCPLAYBACK)
    /* Enable fixed rate with period (ns) 120 +30*700*(2048^0) = 21.1 us (~47.4 kHz) - Generated 1000 times */
    tiSoftTrig(2,100,700,1);   // playback trigger
#elif defined(TIFIXEDPULSER) && (!defined(FADCPLAYBACK))
    tiSoftTrig(1,100,700,0);   // playback trigger
#endif


  /*  Enable FADC */
  faGEnable(0, 0);

  /* Interrupts/Polling enabled after conclusion of rocGo() */

}

void
rocEnd()
{
#ifdef TIRANDOMPULSER
     /* Disable random trigger */
   tiDisableRandomTrigger();
#endif
#if defined(TIFIXEDPULSER) && defined(FADCPLAYBACK)
     /* Disable Fixed Rate trigger */
   tiSoftTrig(2,0,700,1);
#elif defined(TIFIXEDPULSER) && !defined(FADCPLAYBACK)
   tiSoftTrig(1,0,700,0);
#endif

   int ifa;
   /* disable playback */
#ifdef FADCPLAYBACK
   for(ifa=0; ifa<nfadc; ifa++)
     faPPGDisable(faSlot(ifa));
#endif

   for(ifa=0; ifa<nfadc; ifa++)
     faDisableScalers(faSlot(ifa));
  /* FADC Disable */
  faGDisable(0);

  /* FADC Event status - Is all data read out */
  faGStatus(0);

  tiStatus(0);

  tiPrintEvTypeScalers();
  printf("rocEnd: Ended after %d events\n",tiGetIntCount());

}

void
rocTrigger(int arg)
{
  int ifa = 0, stat, nwords, dCnt;
  unsigned int datascan, scanmask;
  int roType = 2, roCount = 0, blockError = 0;

  roCount = tiGetIntCount();

#ifdef TIFIXEDPULSER
  if(ntrigger==100){
    #ifdef FADCPLAYBACK
    /* Enable fixed rate with period (ns) 120 +30*700*(2048^0) = 21.1 us (~47.4 kHz) - Generated 1000 times */
     tiSoftTrig(2,100,700,1);   // playback trigger
    #else
     tiSoftTrig(1,100,700,0);   // playback trigger
    #endif
    ntrigger = 0;
  }
  ntrigger = ntrigger+1;
  //printf("trigger %d\n",ntrigger);
#endif

#ifdef TIRANDOMPULSER
  int PPG_status;

  for(ifa=0; ifa<nfadc; ifa++){
   //faPPGDisable(faSlot(ifa));
   switch(ntrigger){
    case 0:
        PPG_status = faSetPPG(faSlot(ifa),0 , sdata[0], nsamples);
     break;
    case 1:
        PPG_status = faSetPPG(faSlot(ifa),0 , sdata[1], nsamples);
     break;
    case 2:
        PPG_status = faSetPPG(faSlot(ifa),0 , sdata[2], nsamples);
     break;
    case 3:
        PPG_status = faSetPPG(faSlot(ifa),0 , sdata[3], nsamples);
     break;
    case 4:
        PPG_status = faSetPPG(faSlot(ifa),0 , sdata[4], nsamples);
     break;
   }

   if(PPG_status<0) printf("faSetPPG failed at trigger %d\n",ntrigger);

   //faPPGEnable(faSlot(ifa));
  }

  ntrigger = (ntrigger+1)%5;
  printf("!! test: ntrigger=%d\n",ntrigger);

#endif 

  /* Setup Address and data modes for DMA transfers
   *
   *  vmeDmaConfig(addrType, dataType, sstMode);
   *
   *  addrType = 0 (A16)    1 (A24)    2 (A32)
   *  dataType = 0 (D16)    1 (D32)    2 (BLK32) 3 (MBLK) 4 (2eVME) 5 (2eSST)
   *  sstMode  = 0 (SST160) 1 (SST267) 2 (SST320)
   */
  vmeDmaConfig(2,5,1);

  dCnt = tiReadTriggerBlock(dma_dabufp);
  if(dCnt<=0)
    {
      printf("No data or error.  dCnt = %d\n",dCnt);
    }
  else
    {
      dma_dabufp += dCnt;
    }

  /* fADC250 Readout */
  BANKOPEN(FADC_BANK,BT_UI4,0);

  /* Mask of initialized modules */
  scanmask = faScanMask();
  /* Check scanmask for block ready up to 100 times */
  datascan = faGBlockReady(scanmask, 100);
  stat = (datascan == scanmask);

  if(stat)
    {
      if(nfadc == 1)
	roType = 1;   /* otherwise roType = 2   multiboard reaodut with token passing */
      nwords = faReadBlock(0, dma_dabufp, MAXFADCWORDS, roType);

      /* Check for ERROR in block read */
      blockError = faGetBlockError(1);

      if(blockError)
	{
	  printf("ERROR: Slot %d: in transfer (event = %d), nwords = 0x%x\n",
		 faSlot(ifa), roCount, nwords);

	  for(ifa = 0; ifa < nfadc; ifa++)
	    faResetToken(faSlot(ifa));

	  if(nwords > 0)
	    dma_dabufp += nwords;
	}
      else
	{
	  dma_dabufp += nwords;
	  faResetToken(faSlot(0));
	}
    }
  else
    {
      printf("ERROR: Event %d: Datascan != Scanmask  (0x%08x != 0x%08x)\n",
	     roCount, datascan, scanmask);
    }
  BANKCLOSE;


  /* Check for SYNC Event */
  if(tiGetSyncEventFlag() == 1)
    {
      /* Check for data available */
      int davail = tiBReady();
      if(davail > 0)
	{
	  printf("%s: ERROR: TI Data available (%d) after readout in SYNC event \n",
		 __func__, davail);
/*
	  while(tiBReady())
	    {
	      vmeDmaFlush(tiGetAdr32());
	    }
*/	}

      for(ifa = 0; ifa < nfadc; ifa++)
	{
	  davail = faBready(faSlot(ifa));
	  if(davail > 0)
	    {
	      printf("%s: ERROR: fADC250 Data available after readout in SYNC event \n",
		     __func__, davail);

/*	      while(faBready(faSlot(ifa)))
		{
		  vmeDmaFlush(faGetA32(faSlot(ifa)));
		}
*/	    }
	}
    }

#if defined(TIFIXEDPULSER) && defined(FADCPLAYBACK)
    /* Enable fixed rate with period (ns) 120 +30*700*(2048^0) = 21.1 us (~47.4 kHz) - Generated 1000 times */
    tiSoftTrig(2,100,700,1);   // playback trigger
#elif defined(TIFIXEDPULSER) && (!defined(FADCPLAYBACK))
    tiSoftTrig(1,100,700,0);   // playback trigger
#endif

}

void
rocCleanup()
{

  printf("%s: Reset all FADCs\n",__FUNCTION__);
  faGReset(1);

}

/*
  Local Variables:
  compile-command: "make -k fadc_vxs_list.so"
  End:
 */
