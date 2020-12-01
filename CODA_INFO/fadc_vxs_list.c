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
#define MAX_EVENT_LENGTH   1024*60      /* Size in Bytes */

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

#define BUFFERLEVEL 1
#define BLOCKLEVEL 1
#define TIRANDOMPULSER  // using TI internal random pulser as trigger
//#define TIFIXEDPULSER  // using TI internal fixed pulser as trigger
#define FADCPLAYBACK   // turn on fadc playback feature
#define USE_VTP
#define USE_HELBOARD

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
    tiSetRandomTrigger(2,0xe);  // playback trigger
#elif defined(TIRANDOMPULSER) && (!defined(FADCPLAYBACK))
    tiSetRandomTrigger(1,0xe);
#endif

#if defined(TIFIXEDPULSER) && defined(FADCPLAYBACK)
    /* Enable fixed rate with period (ns) 120 +30*700*(2048^0) = 21.1 us (~47.4 kHz) - Generated 1000 times */
    tiSoftTrig(2,1000,700,0);   // playback trigger
#elif defined(TIFIXEDPULSER) && (!defined(FADCPLAYBACK))
    tiSoftTrig(1,1000,700,0);   // playback trigger
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
     int nsamples = 32*16;  // for 16 channels
     unsigned short sdata[nsamples];
     int nchan,nsam;
     for(nchan=0; nchan<16; nchan++){
        for(nsam=0;nsam<32;nsam++){
          if(nsam<10) sdata[nchan*32+nsam] = 60*nsam+100;
          else sdata[nchan*32+nsam] = -30*nsam+1000;
        }
     }

     faPPGDisable(faSlot(ifa));

     int PPG_status = faSetPPG(faSlot(ifa),0 , sdata, nsamples);
     if(PPG_status<0) printf("faSetPPG failed\n");
     else printf("faSetPPG successfull\n");
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
	
      int scaler_status = faSetScalerBlockInterval(faSlot(ifa),100);
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
//30 Hz t_settle 60 us
  WriteHelBoard(0,5);
  int hel_status1 = WriteHelBoard(1,27);
  WriteHelBoard(2,0);
  WriteHelBoard(3,1);  
  int hel_status4 = WriteHelBoard(4,3);
 

  if(hel_status1==0 && hel_status4==0) printf("Helicity board write successfully\n");
  else printf("Helicity board write failed\n");
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
   tiSoftTrig(2,0,700,0);
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

  printf("rocEnd: Ended after %d events\n",tiGetIntCount());

}

void
rocTrigger(int arg)
{
  int ifa = 0, stat, nwords, dCnt;
  unsigned int datascan, scanmask;
  int roType = 2, roCount = 0, blockError = 0;

  roCount = tiGetIntCount();

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
