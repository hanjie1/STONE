typedef struct trigBankObject {
  int      blksize;              /* total number of triggers in the Bank */
  uint16_t tag;                  /* Trigger Bank Tag ID = 0xff2x */
  uint16_t nrocs;                /* Number of ROC Banks in the Event Block (val = 1-256) */
  uint32_t len;                  /* Total Length of the Trigger Bank - including Bank header */
  int      withTimeStamp;        /* =1 if Time Stamps are available */
  int      withRunInfo;          /* =1 if Run Informaion is available - Run # and Run Type */
  uint64_t evtNum;               /* Starting Event # of the Block */
  uint64_t runInfo;              /* Run Info Data */
  uint32_t *start;               /* Pointer to start of the Trigger Bank */
  uint64_t *evTS;                /* Pointer to the array of Time Stamps */
  uint16_t *evType;              /* Pointer to the array of Event Types */
  int      helicity;			 /* helicity in misc data */
  int      mps;			 /* mps in misc data */
} TBOBJ;

TBOBJ tbank;

int trigBankDecode(uint32_t *tb, int blkSize)
{

  memset((void *)&tbank, 0, sizeof(TBOBJ));

  tbank.start = (uint32_t *)tb;
  tbank.blksize = blkSize;
  tbank.len = tb[0] + 1;                    //buf[2]
  tbank.tag = (tb[1]&0xffff0000)>>16;
  tbank.nrocs = (tb[1]&0xff);
  tbank.evtNum = tb[3];

  if((tbank.tag)&1)
    tbank.withTimeStamp = 1;
  if((tbank.tag)&2)
    tbank.withRunInfo = 1;

  int index_roc=5;

  if(tbank.withTimeStamp) {
    tbank.evTS = (uint64_t *)&tb[5];
    if(tbank.withRunInfo) {
      tbank.evType = (uint16_t *)&tb[5 + 2*blkSize + 3];
	  index_roc = 5 + 2*blkSize + 3;
    }else{
      tbank.evType = (uint16_t *)&tb[5 + 2*blkSize + 1];
	  index_roc = 5 + 2*blkSize + 1;
    }
  }else{
    tbank.evTS = NULL;
    if(tbank.withRunInfo) {
      tbank.evType = (uint16_t *)&tb[5 + 3];
	  index_roc = 8;
    }else{
      tbank.evType = (uint16_t *)&tb[5 + 1];
	  index_roc = 6;
    }
  }

  index_roc += 1;
  for(uint32_t ii=index_roc;ii<tbank.len;ii++){
      if((tb[ii]&0xffff0000)==0xda560000){
		  tbank.helicity = (tb[ii] & 0x20)>>5;
		  tbank.mps = (tb[ii] & 0x10)>>4;
		  break;
      }
      else continue;
  }

  return(tbank.len);
}
                      
