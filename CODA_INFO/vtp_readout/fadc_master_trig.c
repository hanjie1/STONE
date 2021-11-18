/*************************************************************************
 *
 *  ti_master_list.c - Library of routines for readout and buffering of 
 *                     events using a JLAB Trigger Interface V3 (TI) with 
 *                     a Linux VME controller in CODA 3.0.
 *
 *                     This for a TI in Master Mode controlling multiple ROCs
 */

/* Event Buffer definitions */
#define MAX_EVENT_POOL     100
#define MAX_EVENT_LENGTH   1152*32      /* Size in Bytes */

/* Define maximum number of words in the event block
   MUST be less than MAX_EVENT_LENGTH/4   */
#define MAX_WORDS 200

/* Define TI Type (TI_MASTER or TI_SLAVE) */
#define TI_MASTER
/* EXTernal trigger source (e.g. front panel ECL input), POLL for available data */
#define TI_READOUT TI_READOUT_EXT_POLL 
/* TI VME address, or 0 for Auto Initialize (search for TI by slot) */
#define TI_ADDR  0           

/* Disable Streaming mode */
#undef STREAMING_MODE
/* Enable VXS Readout */
#define FADC_VXS_READOUT
#define INTRANDOMPULSER 

/* Measured longest fiber length in system */
#define FIBER_LATENCY_OFFSET 0x4A  

#include "dmaBankTools.h"   /* Macros for handling CODA banks */
#include "tiprimary_list.c" /* Source required for CODA readout lists using the TI */
#include "fadcLib.h"        /* library of FADC250 routines */
#include "sdLib.h"          /* VXS Signal Distribution board header */

/* Define initial blocklevel and buffering level */
#define BLOCKLEVEL  5
#define BUFFERLEVEL 5
#define SYNC_INTERVAL 100000

/* FADC Library Variables */
extern int fadcA32Base, nfadc;
/* Program FADCs to get trigger/clock from SDC or VXS */
#define FADC_VXS

#define NFADC     1
/* Address of first fADC250 */
#define FADC_ADDR (3<<19)
/* Increment address to find next fADC250 */
#define FADC_INCR 0x010000

/* Set a common FADC Threshhold for all channels */
//#define FADC_THRESHOLD  300
#define FADC_THRESHOLD  101

#define FADC_WINDOW_LAT    665
#define FADC_WINDOW_WIDTH  32
#define FADC_MODE           1   /* Hall B modes 1-8 are supported*/

/* for the calculation of maximum data words in the block transfer */
unsigned int MAXFADCWORDS=0;
#define nsamples 32*16
unsigned short sdata[nsamples];


/****************************************
 *  DOWNLOAD
 ****************************************/
void
rocDownload()
{
  int stat;

  /* Setup Address and data modes for DMA transfers
   *   
   *  vmeDmaConfig(addrType, dataType, sstMode);
   *
   *  addrType = 0 (A16)    1 (A24)    2 (A32)
   *  dataType = 0 (D16)    1 (D32)    2 (BLK32) 3 (MBLK) 4 (2eVME) 5 (2eSST)
   *  sstMode  = 0 (SST160) 1 (SST267) 2 (SST320)
   */
  vmeDmaConfig(2,5,1); 

  /* Define BLock Level */
  blockLevel = BLOCKLEVEL;
  bufferLevel = BUFFERLEVEL;


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
//  tiSetTriggerSource(TI_TRIGGER_TSINPUTS); /* TS Inputs enabled */
  tiSetTriggerSourceMask(TI_TRIGSRC_PULSER | TI_TRIGSRC_LOOPBACK | TI_TRIGSRC_TRIG21); 

  /* Enable set specific TS input bits (1-6) */
  //tiEnableTSInput( TI_TSINPUT_1 | TI_TSINPUT_2 );

  /* Load the trigger table that associates 
   *  pins 21/22 | 23/24 | 25/26 : trigger1
   *  pins 29/30 | 31/32 | 33/34 : trigger2
   */
  tiLoadTriggerTable(0);

  tiSetTriggerHoldoff(1,10,0);
  tiSetTriggerHoldoff(2,10,0);

  /* Set the sync delay width to 0x40*32 = 2.048us */
  tiSetSyncDelayWidth(0x54, 0x40, 1);
  
  /* Set initial number of events per block */
  tiSetBlockLevel(blockLevel);

  /* Set Trigger Buffer Level */
  tiSetBlockBufferLevel(BUFFERLEVEL);

  /* Init the SD library so we can get status info */
  stat = sdInit(0);
  if(stat==0) 
    {
      tiSetBusySource(TI_BUSY_SWB,1);
      sdSetActiveVmeSlots(0);
      sdStatus(0);
    }
  else
    { /* No SD or the TI is not in the Proper slot */
      tiSetBusySource(TI_BUSY_LOOPBACK,1);
    }

#ifndef STREAMING_MODE
#ifdef FADC_VXS_READOUT
  tiRocEnable(2);  /* Make sure the VTP trigger acknowledge busy is enabled */
#endif
#endif


  tiStatus(0);


  printf("rocDownload: User Download Executed\n");

}

/****************************************
 *  PRESTART
 ****************************************/
void
rocPrestart()
{
  unsigned short iflag;
  unsigned int fadcmask=0;
  unsigned int en_mask, ival = SYNC_INTERVAL;
  int ifa, stat;
  int islot;


  /* Unlock the VME Mutex */
  vmeBusUnlock();

#ifndef FADC_VXS  
  /* Init FADC Library and modules here id using a SDC board in a non-VXS crate */
  iflag = 0xea00; /* SDC Board address A16 */
  iflag |= FA_INIT_EXT_SYNCRESET;  /* Front panel sync-reset */
  iflag |= FA_INIT_FP_TRIG;  /* Front Panel Input trigger source */
  iflag |= FA_INIT_FP_CLKSRC;  /* Internal 250MHz Clock source */
#else
  /* If this is in a VXS crate Get CLK, TRIG, and Sync Reset from VXS */
  iflag = 0x25;
#endif

  /* Initialize FADC library */
  fadcA32Base = 0x09000000;
  vmeSetQuietFlag(0);
  faInit(FADC_ADDR, FADC_INCR, NFADC, (iflag|FA_INIT_SKIP_FIRMWARE_CHECK));
  vmeSetQuietFlag(0);


  /* Program/Init VME Modules Here */
  for(ifa=0; ifa < nfadc; ifa++) 
    {
      /* generate a slot mask */
      fadcmask |= (1<<faSlot(ifa));

      faSoftReset(faSlot(ifa),0);
      faResetTriggerCount(faSlot(ifa));

      faEnableSyncReset(faSlot(ifa));

      faEnableBusError(faSlot(ifa));

      /* Set input DAC level - 3250 basically corresponds to 0 shift in the baseline. */
      faSetDAC(faSlot(ifa), 3250, 0);

      /*  Set All channel thresholds to 300 */
      faSetThreshold(faSlot(ifa), FADC_THRESHOLD, 0xffff);

#ifdef FADC_VXS_READOUT
      /* Redirected triggered data to VXS - and bypass VME */
      faSetVXSReadout(faSlot(ifa), 1);
#endif
  
      /*********************************************************************************
       * faSetProcMode(int id, int pmode, unsigned int PL, unsigned int PTW, 
       *    int NSB, unsigned int NSA, unsigned int NP, 
       *    unsigned int NPED, unsigned int MAXPED, unsigned int NSAT);
       *
       *  id    : fADC250 Slot number
       *  pmode : Processing Mode
       *          9 - Pulse Parameter (ped, sum, time)
       *         10 - Debug Mode (9 + Raw Samples)
       *    PL : Window Latency
       *   PTW : Window Width

       *   NSB : Number of samples before pulse over threshold
       *   NSA : Number of samples after pulse over threshold
       *    NP : Number of pulses processed per window
       *  BANK : (Hall B option - replaces NPED,MAXPED,NSAT)
       *  NPED : Number of samples to sum for pedestal (4)
       *MAXPED : Maximum value of sample to be included in pedestal sum (250)
       *  NSAT : Number of consecutive samples over threshold for valid pulse (2)
       */
      faSetProcMode(faSlot(ifa),
		    FADC_MODE,   /* Processing Mode */
		    FADC_WINDOW_LAT, /* PL */
		    FADC_WINDOW_WIDTH,  /* PTW */
		    3,   /* NSB */
		    6,   /* NSA */
		    1,   /* NP */
		    0   /* BANK */
		    );

        /* set fadc scaler */    
        int scaler_status = faSetScalerBlockInterval(faSlot(ifa),1); 
	if(scaler_status<0) printf("faSetScalerBlockInterval failed\n");
	else printf("faSetScalerBlockInterval successfull\n"); 

        int nchan,nsam;

        for(nchan=0; nchan<16; nchan++){ // chan 0-15 initial pedestals
          for(nsam=0;nsam<32;nsam++){
            if(nsam<5) sdata[nchan*32+nsam] = 225*nsam+100;
	    else{
              if(nsam>=5 && nsam<15) 
		sdata[nchan*32+nsam] = 1000-90*(nsam-4);
	      else
		sdata[nchan*32+nsam]=100;
	    } 
	//    printf("sdata[%d]=%d\n",nchan*32+nsam,sdata[nchan*32+nsam]);
          }
        }

        faPPGDisable(faSlot(ifa));

        int PPG_status = faSetPPG(faSlot(ifa),0 , sdata, nsamples);
        if(PPG_status<0) printf("faSetPPG failed\n");
        else printf("faSetPPG initialize successfull\n");

        faPPGEnable(faSlot(ifa)); 
        faEnableScalers(faSlot(ifa)); 

    }

  /* Print status for FADCS*/
#ifndef FADC_VXS
  faSDC_Status(0);
#endif
  faGStatus(0);


  /* Set number of events per block (broadcasted to all connected TI Slaves)*/
  tiSetBlockLevel(blockLevel);
  printf("rocPrestart: Block Level set to %d\n",blockLevel);

  /* Reset Active ROC Masks on all TD modules */
  tiTriggerReadyReset();

  /* Set Sync Event Interval  (0 disables sync events, max 65535) */
  tiSetSyncEventInterval(ival);
  printf("rocPrestart: Set Sync interval to %d Blocks\n",ival);

  /* Program the SD to look for Busy from the Active FADC slots */
  if(fadcmask) {
    printf("rocPrestart: Set Active Busy mask for SD (0x%06x)\n",fadcmask);
    sdSetActiveVmeSlots(fadcmask);
  }
  
  sdStatus(0);
  tiStatus(0); 


#ifdef FADC_VXS_READOUT
  printf(" *** WARNING, FADC VXS READOUT is enabled for this Configuration ****\n");
  en_mask = faGetVXSReadout(0);
  printf(" FADC 0  faGetVXSReadout = %d\n",en_mask);
#endif

  printf("rocPrestart: User Prestart Executed\n");

}

/****************************************
 *  GO
 ****************************************/
void
rocGo()
{
  int ii, islot, ntrig_busy;
  unsigned int tmask, en_mask;

  /* Get the current Block Level */
  blockLevel = tiGetCurrentBlockLevel();
  printf("rocGo: Block Level set to %d\n",blockLevel);

  /* Enable Slave Ports that have indicated they are active */
  tiResetSlaveConfig();
  tmask = tiGetTrigSrcEnabledFiberMask();
  printf("%s: TI Source Enable Mask = 0x%x\n", __func__, tmask);
  if (tmask != 0)
    tiAddSlaveMask(tmask);

  /* Enable/Set Block Level on modules, if needed, here */
  faGSetBlockLevel(blockLevel);


#ifndef STREAMING_MODE
  /* Enable FADC Busy feedback to the TI */
  ntrig_busy=6;
  for(ii=0;ii<nfadc;ii++) {
    faSetTriggerBusyCondition(faSlot(ii),ntrig_busy);
  }
#endif

  if(FADC_MODE == 9)
    MAXFADCWORDS = 2 + 4 + blockLevel * 8;
  else /* FADC_MODE == 10 */
    /* MAXFADCWORDS = 2 + 4 + blockLevel * (8 + FADC_WINDOW_WIDTH/2); */
    MAXFADCWORDS = 2000;

  /*  Enable FADCs */
  faGEnable(0, 0);


  /* Example: How to start internal pulser trigger */
#ifdef INTRANDOMPULSER
  /* Enable Random at rate 500kHz/(2^7) = ~3.9kHz */
  tiSetRandomTrigger(2,0x9);
  //tiSetRandomTrigger(1,0x9);
  tiSetTrig21Delay(0);
#elif defined (INTFIXEDPULSER)
  /* Enable fixed rate with period (ns) 120 +30*700*(1024^0) = 21.1 us (~47.4 kHz)
     - Generated 1000 times */
  tiSoftTrig(1,1000,700,0);
#endif
}

/****************************************
 *  END
 ****************************************/
void
rocEnd()
{
  int islot;
  int ifa;

  /* Example: How to stop internal pulser trigger */
#ifdef INTRANDOMPULSER
  /* Disable random trigger */
  tiDisableRandomTrigger();
#elif defined (INTFIXEDPULSER)
  /* Disable Fixed Rate trigger */
  tiSoftTrig(1,0,700,0);
#endif

  /* FADC Disable */
  for(ifa=0; ifa<nfadc; ifa++) 
      faPPGDisable(faSlot(ifa));

  for(ifa=0; ifa<nfadc; ifa++)
      faDisableScalers(faSlot(ifa));

  faGDisable(0);

  /* FADC Event status - Is all data read out */
  faGStatus(0);

  tiStatus(0);

  printf("rocEnd: Ended after %d blocks\n",tiGetIntCount());
  
}

/****************************************
 *  TRIGGER
 ****************************************/
void
rocTrigger(int arg)
{
  int ii, islot;
  int stat, ifa, nwords, blockError, dCnt, len=0, idata;
  unsigned int val;
  unsigned int *start;
  unsigned int datascan, scanmask, roCount;

  /* Set TI output 1 high for diagnostics */
  tiSetOutputPort(1,0,0,0);

  /* Readout the trigger block from the TI 
     Trigger Block MUST be reaodut first */
  dCnt = tiReadTriggerBlock(dma_dabufp);

  if(dCnt<=0)
    {
      printf("No TI Trigger data or error.  dCnt = %d\n",dCnt);
    }
  else
    { /* TI Data is already in a bank structure.  Bump the pointer */
      dma_dabufp += dCnt;
    }


#ifndef FADC_VXS_READOUT
  /* fADC250 Readout */
  BANKOPEN(3,BT_UI4,blockLevel);

  /* Mask of initialized modules */
  scanmask = faScanMask();
  /* Check scanmask for block ready up to 100 times */
  datascan = faGBready(scanmask, 100); 
  stat = (datascan == scanmask);

  if(stat) 
    {
      for(ifa = 0; ifa < nfadc; ifa++)
	{
	  nwords = faReadBlock(faSlot(ifa), dma_dabufp, MAXFADCWORDS, 1);
	  
	  /* Check for ERROR in block read */
	  blockError = faGetBlockError(1);
	  
	  if(blockError) 
	    {
	      printf("ERROR: Slot %d: in transfer (event = %d), nwords = 0x%x\n",
		     faSlot(ifa), roCount, nwords);

	      if(nwords > 0)
		dma_dabufp += nwords;
	    } 
	  else 
	    {
	      dma_dabufp += nwords;
	    }
	}
    }
  else 
    {
      printf("ERROR: Event %d: Datascan != Scanmask  (0x%08x != 0x%08x)\n",
	     roCount, datascan, scanmask);
      *dma_dabufp++ = 0xda0bad003;
      *dma_dabufp++ = roCount;
      *dma_dabufp++ = datascan;
      *dma_dabufp++ = scanmask;
    }
  BANKCLOSE;

#else

  /* Open a bank (type=5) and add data words by hand */
  BANKOPEN(5,BT_UI4,blockLevel);
  *dma_dabufp++ = tiGetIntCount();
  *dma_dabufp++ = 0xdead;
  *dma_dabufp++ = 0xcebaf111;
  *dma_dabufp++ = 0xcebaf222;
  BANKCLOSE;

#endif



  /* Set TI output 0 low */
  tiSetOutputPort(0,0,0,0);

}

void
rocCleanup()
{
  int islot=0;

  printf("%s: Reset all Modules\n",__FUNCTION__);
  
  /* Reset the FADCs */
  faGReset(1);

}
