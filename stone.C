#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <iostream>
#include <fstream>
#include "TROOT.h"
#include "TFile.h"
#include "TTree.h"
#include "TString.h"
#include "TBranch.h"
#include "TH1.h"

#include "evio.h"
#include "simpleLib.h"
#include "SetParams.h"
#include "SetTreeVars.h"
#include "Fadc250Decode.h"
#include "VTPDecode.h"
#include "FindHelicity.h"

using namespace std;

// global parameters (eg. maxroc, max fadc channels ...) are set in SetParams.h
// tree variables are set in SetTreeVars.h

unsigned int LSWAP(unsigned int data);
void ClearScaler();
void ClearTreeVar();
bool verbose = false; 
Int_t scaldat[32]={0};

int main ()
{
  int run_number=0;
  int indx, bt, dt, blk, handle, status, nWords, version;
  ULong64_t nevents;
  int pe;
  unsigned int blocklevel = 1;
  uint32_t *buf, dLen, bufLen;
  char *dictionary = NULL;
  bool eventbyevent = true; 
  ULong64_t maxevents = 5e9;
  ULong64_t totalmax=1000;
  bool firstevent = true;
  int check;
  bool totaldone = false;
  int ndatafile = 0;
   

  cout<<"Which run? ";
  cin>>run_number;
  // Initialize root and output 
  TString outfile=Form("Rootfiles/fadctest_%d.root",run_number);

  TFile *hfile = new TFile(outfile,"RECREATE","e detector data");
  //if(!hfile->IsOpen()) return;  
  //Build TTree to store TDC data
  TTree *T = new TTree("T","data");
  T->Branch("evtype", &evtype, "evtype/I"); 
  T->Branch("ti_timestamp", &ti_timestamp, "ti_timestamp/l"); 
  T->Branch("fadc_trigtime", &fadc_trigtime, "fadc_trigtime/l"); 
  T->Branch("fadc_mode", &fadc_mode, "fadc_mode/I"); 
  T->Branch("fadc_a", fadc_int, Form("fadc_int[%i]/I",FADC_NCHAN)); 
  T->Branch("fadc_t", fadc_time, Form("fadc_time[%i]/I",FADC_NCHAN)); 
  T->Branch("fadc_a1", fadc_int_1, Form("fadc_int_1[%i]/I",FADC_NCHAN)); 
  T->Branch("fadc_t1", fadc_time_1, Form("fadc_time_1[%i]/I",FADC_NCHAN)); 
  T->Branch("fadc_nhit", fadc_nhit, Form("fadc_nhit[%i]/I",FADC_NCHAN)); 
  T->Branch("fadc_scal_cnt", fadc_scal_cnt, "fadc_scal_cnt[16]/I"); 
  T->Branch("fadc_scal_rate", fadc_scal_rate, "fadc_scal_rate[16]/I"); 
  T->Branch("fadc_scal_time", &fadc_scal_time, "fadc_scal_time/I"); 
  T->Branch("fadc_scal_trigcnt", &fadc_scal_trigcnt, "fadc_scal_trigcnt/I"); 

  TTree *VTP = new TTree("VTP","vtp data");
  VTP->Branch("vtp_trigtime", &vtp_trigtime, "vtp_trigtime/l");
  VTP->Branch("busytime",&busytime,"busytime/I");
  VTP->Branch("livetime",&livetime,"livetime/I");
  VTP->Branch("hel_win_cnt_1",&hel_win_cnt_1,"hel_win_cnt_1/I");
  VTP->Branch("trigcnt",trigcnt,"trigcnt[5]/I");
  VTP->Branch("pattern_num",&pattern_num,"pattern_num/I");
  VTP->Branch("trig_pattern",trig_pattern,"trig_pattern[pattern_num]/I");
  VTP->Branch("trig_pattern_time",trig_pattern_time,"trig_pattern_time[pattern_num]/I");
  VTP->Branch("last_mps_time",&last_mps_time,"last_mps_time/I");
  VTP->Branch("hel_win_cnt",&hel_win_cnt,"hel_win_cnt/I");
  VTP->Branch("vtp_past_hel",vtp_past_hel,"vtp_past_hel[6]/I");
  VTP->Branch("vtp_helicity", &vtp_helicity, "vtp_helicity/I");
  VTP->Branch("vtp_fadc_scalcnt",vtp_fadc_scalcnt,Form("vtp_fadc_scalcnt[%d]/I",FADC_NCHAN));



  nevents=1;
  /* Open file  */
  while(ndatafile<20){ // loop all data files
  char datapath[100];
  sprintf(datapath,"/home/daq/tmpdata/fadctest/fadc_test_%d.dat.%d",run_number,ndatafile);

  ifstream infile(datapath);
  if(!infile){
	printf("Can't find file %s....stop here\n",datapath);
	break;
  }

  if ( (status = evOpen(datapath, (char*)"r",  &handle)) < 0) 
  {
    printf("Unable to open file %s status = %d\n",datapath,status);
    exit(-1);
  } 
  else
	printf("Open file /home/daq/data/fadctest/fadc_test_%d.dat.%d\n",run_number,ndatafile);

  ndatafile++;

  /* Get evio version # of file */
  status = evIoctl(handle, (char*)"v", &version);
  if (status == S_SUCCESS) 
  {
    printf("Evio file version = %d\n\n", version);
  }

  /* Get a dictionary if there is one */
  status = evGetDictionary(handle, &dictionary, &dLen);
  if (status == S_SUCCESS && dictionary != NULL) 
  {
    printf("Dictionary =\n%s\n\n", dictionary);
    free(dictionary);
  }

  // Unblocking initialize
  // SIMPLE: initialize
  simpleInit();

  /* SIMPLE: Define banks For compton
   *  rocID     bankID     slot   what      endian
   *  1         3            3    fadc250   little
   *  1         4           13    vetroc    little
   *  1         4           14    vetroc    little
   *  1         4           15    vetroc    little
   *  1         4           16    vetroc    little
   *  1         6           ??    ??
   *  3         0x11        ??    ??
   *  3         0x12        ??    ??
   *  3         0x56        11    vtp       big
   *
   *
   *  int
   *  simpleConfigBank(int rocID, int bankID, int num,
   *                    int endian, int isBlocked, void *firstPassRoutine)
   *
   * @param rocID             roc ID
   * @param bankID            Bank ID
   * @param num               NOT USED
   * @param endian            little = 0, big = 1
   * @param isBlocked         no = 0, yes = 1
   * @param firstPassRoutine  Routine to call for first pass processing
   */
  simpleConfigBank(1, 0x3, 0, 0, 1, NULL);
  simpleConfigBank(3, 0x56, 0, 1, 1, NULL);

  /* Loop through getting event blocks one at a time and print basic infomation
     about each block */
  while ((status = evReadAlloc(handle, &buf, &bufLen))==0) 
    //while ((evReadAlloc(handle, &buf, &bufLen))!= EOF && (evReadAlloc(handle, &buf, &bufLen))==0) 
  { /* read the event and allocate the correct size buffer */
    indx=0; pe=0;
    nWords = buf[0] + 1;
    bt  = ((buf[1]&0xffff0000)>>16);  /* Bank Tag */
    dt  = ((buf[1]&0xff00)>>8);       /* Data Type */
    blk = buf[1]&0xff;                /* Event Block size */

    if(verbose) printf("    BLOCK #%llu,  Bank tag = 0x%04x, Data type = 0x%04x,  Total len = %d words\n", nevents, bt, dt, nWords);

    /* Check on what type of event block this is */
    if((bt >= 0xff00)> 0) 
    {/* CODA Reserved bank type */
      switch (bt) {
        case 0xffd1:
          if(verbose) printf("    ** Prestart Event **\n");
          break;
        case 0xffd2:
          if(verbose) printf("    ** Go Event **\n");
          break;
        case 0xffd4:
          if(verbose) printf("    ** End Event **\n");
          break;
        case 0xff50:
        case 0xff58:
        case 0xff70:
          if(verbose) printf("    ** Physics Event Block (%d events in Block) **\n",blk);
          pe=1;
          break;
        default:
          if(verbose) printf("    ** Undefined CODA Event Type **\n");
      }
    }
    else
    { /* User event type */
      printf("    ** User Event (Type = %d) **\n",bt);
    }

    if (pe == 0) 
    {
      indx += nWords;
    } 
    else 
    { /* This is a built Physics Event. Disect a bit more... */

	  /**  Scan data to find blocks and banks **/
      simpleScan(buf, nWords);

      indx += 2;

      /**  Get trigger bank buf **/
      int tbLen1 = 0; // trigger bank time segment
      int tbLen2 = 0; // trigger bank type segment
      int tbLenROC[2] = {0}; // trigger bank ROC segment
   
      unsigned long long *simpTrigBuf1 = NULL;
      tbLen1 = simpleGetTriggerBankTimeSegment(&simpTrigBuf1);
      ULong64_t fevtNum = simpTrigBuf1[0];
      if(fevtNum != nevents){
	 printf("The event number %llu from TI does not match the counter %llu !\n",fevtNum,nevents);
	 break;
      }

      if(verbose)printf("Event number for the first event in the block = %llu \n",fevtNum);

      unsigned short *simpTrigBuf2 = NULL;
      tbLen2 = simpleGetTriggerBankTypeSegment(&simpTrigBuf2);

      unsigned int *simpTrigRocBuf1 = NULL;
      tbLenROC[0] = simpleGetTriggerBankRocSegment(TI_ROC,&simpTrigRocBuf1); // TI ROC

      unsigned int *simpTrigRocBuf2 = NULL;
      tbLenROC[1] = simpleGetTriggerBankRocSegment(VTP_ROC,&simpTrigRocBuf2); // VTP ROC

      if(verbose)printf("time len = %d , type len = %d , roc1 len = %d , roc2 len = %d\n",tbLen1,tbLen2,tbLenROC[0],tbLenROC[1]);

      int BLOCKLEVEL=1;
      check = simpleGetRocBlockLevel(TI_ROC, FADC_BANK, &BLOCKLEVEL);
      blocklevel = BLOCKLEVEL;
      if(check == -1)printf("Couldn't find block level !\n");
      if(verbose)printf("Block level = %d\n",blocklevel);

      unsigned int header = 0;
      /** VTP block header **/
      check = simpleGetSlotBlockHeader(VTP_ROC, VTP_BANK, VTP_SLOT, &header);
      if(check <= 0)
         printf("ERROR getting VTP block header\n");
      else{
         vtpDataDecode(LSWAP(header));
         if(verbose)printf("VTP evt blk = %d, num of evts = %d\n",vtp_data.blk_num, vtp_data.blk_size);
         if(vtp_data.blk_size != blocklevel)
         printf("VTP block size %d is not equal to blocklevel %d !\n",vtp_data.blk_size, blocklevel);
      }

      /* FADC block header */
      check = simpleGetSlotBlockHeader(TI_ROC, FADC_BANK, FADC_SLOT, &header);
      if(check <= 0) 
	printf("ERROR getting FADC block header\n");
      else{
       faDataDecode(header);
       if(verbose)printf("FADC evt blk = %d, num of evts = %d\n",fadc_data.blk_num, fadc_data.n_evts);
       if(fadc_data.n_evts != blocklevel)
	 printf("ERROR fadc number of events %d is not equal to blocklevel %d !\n",fadc_data.n_evts, blocklevel);
      }

      /**  loop blocks **/
      for(int ii=0;ii<BLOCKLEVEL;ii++){ 
	  ClearTreeVar();
	  if(firstevent)fadc_mode=-1;

	  if(nevents>totalmax && totalmax != 1){
		 totaldone = true;
		 break;
	  }
          if(nevents%1000000==0) printf("  Event number = %llu **\n",nevents);
         
	  ti_timestamp = simpTrigBuf1[ii+1]; 
	  evtype = simpTrigBuf2[ii];
	  unsigned int tmpdata;
	  tmpdata = simpTrigRocBuf1[ii*3+2];	  
	  if((tmpdata & 0xffff0000)== 0xda560000){
		tMPS = (tmpdata & 0x10)>>4;
	  }
	  else
		printf("Couldn't find helicity bits !!\n");

          unsigned int *simpDataBuf = NULL;
	  int simpLen=0;
	  /** FADC event data **/
          simpLen = simpleGetSlotEventData(TI_ROC, FADC_BANK, FADC_SLOT, ii, &simpDataBuf);
	  if(simpLen <= 0)
		printf("ERROR fadc event data length %d <= 0 \n",simpLen);
          else{
		for(int idata = 0; idata < simpLen; idata++)
		   faDataDecode(simpDataBuf[idata]);

	           if(firstevent){
		      fadc_mode = GetFadcMode();
	   	      if(fadc_mode == RAW_MODE)
                         T->Branch("fadc_rawADC", frawdata, Form("frawdata[%i][%i]/I",FADC_NCHAN,MAXRAW)); 

		      fadc_scal_pretime=0;
		      for(int kk=0; kk<16; kk++) fadc_scal_precnt[kk]=0;
		   }

		   if(fadc_scal_update==1){
		      Double_t delta_t = (fadc_scal_time-fadc_scal_pretime)*2048.0*1e-9;  // s
		      if(delta_t<=0) printf("ERROR: FADC scaler timer is not updated\n");

		      for(int kk=0; kk<16; kk++){
			Double_t delta_cnt = 1.0*( fadc_scal_cnt[kk]-fadc_scal_precnt[kk] );
			if(delta_t>0) fadc_scal_rate[kk]=delta_cnt/delta_t;

		        fadc_scal_precnt[kk] = fadc_scal_cnt[kk];	
		      }
		      fadc_scal_pretime = fadc_scal_time;
		   }
	      }

            /**  VTP event data **/
          simpLen = 0;
          simpDataBuf = NULL;
          simpLen = simpleGetSlotEventData(VTP_ROC, VTP_BANK, VTP_SLOT, ii, &simpDataBuf);
          if(simpLen <= 0)
             printf("ERROR vtp event data length %d <= 0 \n",simpLen);
          else{
             for(int idata = 0; idata < simpLen; idata++){
                 unsigned int new_data;
                 new_data = LSWAP(simpDataBuf[idata]);
                 vtpDataDecode(new_data);
             }
             for(int mm=0;mm<6;mm++)
                vtp_past_hel[mm] = vtp_data.helicity[mm];

             //vtp_helicity = InvertBit((vtp_past_hel[0] & 0x1));   // most recent helicity seen by VTP
             vtp_helicity = (vtp_past_hel[0] & 0x1);   // most recent helicity seen by VTP
          }

          T->Fill();
 	  VTP->Fill();
          nevents++;

	  if(firstevent) firstevent = false;

          if(eventbyevent) {
      		printf("Hit return for next event or q to exit; hit a or A to replay all events or certain number of events.\n");
      		int typein = getchar(); 
      		if(typein == 113){ totaldone= true; break;} 
      		if(typein == 65 || typein == 97){
		  eventbyevent=false;
	    	  cout<<"How many events? (hit 1 for total;)";
		  cin>>totalmax;
	  	}
    	   }	

    	   if(nevents > maxevents) {
      		printf("Completed %llu events!\n", nevents-1); 
      		totaldone=true;
		    break;	
           }
	  } // loop over block levels


      if(totaldone) break;
    }

    /* free the event buffer and wait for next one */
    free(buf);

  } // End of loop one data file

  if(totaldone) break;
  if ( status == EOF ) 
  {
    printf("Found end-of-file; total %llu events. \n", nevents);
  }
  else if(status != 0)
  {
    printf("Error reading file (status = %d, quit\n",status);
	exit(-1);
  }
  evClose(handle);
  }  // loop all data files

  T->Write(); 
  VTP->Write(); 
  hfile->Close(); 
 // evClose(handle);


  exit(0);

} // End of main function

unsigned int LSWAP(unsigned int data){
	   unsigned int new_data;
	   new_data = ((data & 0x000000ff)<<24) |
		 		  ((data & 0x0000ff00)<<8)  |
				  ((data & 0x00ff0000)>>8)  |
				  ((data & 0xff000000)>>24) ;
	   return new_data;
}

void ClearTreeVar(){

 tHelicity=0;  
 tMPS=0;	   
 evtype=0;    
 ti_timestamp = 0;

 memset(fadc_int, 0, FADC_NCHAN*sizeof(fadc_int[0]));
 memset(fadc_time, 0, FADC_NCHAN*sizeof(fadc_time[0]));
 memset(fadc_int_1, 0, FADC_NCHAN*sizeof(fadc_int_1[0]));
 memset(fadc_time_1, 0, FADC_NCHAN*sizeof(fadc_time_1[0]));
 memset(fadc_nhit, 0, FADC_NCHAN*sizeof(fadc_nhit[0]));
 memset(ftdc_nhit, 0, FADC_NCHAN*sizeof(ftdc_nhit[0]));
 memset(frawdata, 0, FADC_NCHAN*MAXRAW*sizeof(frawdata[0][0]));	
 fadc_trigtime = 0;
 nrawdata=0;

 memset(fadc_scal_cnt, 0, 16*sizeof(fadc_scal_cnt[0]));
 fadc_scal_time=0;
 fadc_scal_trigcnt=0;

 fadc_scal_update=0;
 memset(fadc_scal_rate, 0, 16*sizeof(fadc_scal_rate[0]));

 ClearVTP();  // clear some vtp_data 
 vtp_trigtime = 0;
 busytime = 0;
 livetime = 0;
 hel_win_cnt_1 = 0;
 memset(trigcnt, 0, 5*sizeof(trigcnt[0]));
 pattern_num = 0;
 memset(trig_pattern, 0, 64*sizeof(trig_pattern[0]));
 memset(trig_pattern_time, 0, 64*sizeof(trig_pattern_time[0]));
 last_mps_time = 0;
 hel_win_cnt = 0;
 vtp_helicity = 0;
 memset(vtp_past_hel, 0, 6*sizeof(vtp_past_hel[0]));
 memset(vtp_fadc_scalcnt, 0, FADC_NCHAN*sizeof(vtp_fadc_scalcnt[0]));

}
