#include "/home/daq/work/SOLID_DAQ/decoder/SetParams.h"
#include "FindHelicity.h"
#include "FindPulses.h"

void CalcAsym(){

    int nrun=0;
    cout<<"Which run?  ";
    cin>>nrun;

    TString filename = Form("/home/daq/work/SOLID_DAQ/decoder/Rootfiles/fadctest_%d.root", nrun);

    TFile *f0 = new TFile(filename);
    TTree *T = (TTree*) f0->Get("T");
    TTree *VTP = (TTree*) f0->Get("VTP");
    T->AddFriend(VTP);

    int NPED[16]={700,0,2000,0,0,0,0,0,0,0,0,0,0,0,0,0};
    int width=30; 

    Int_t fadc_scal[16],fadc_a[16],fadc_scal_trigcnt,vtp_trigcnt[5];
    Int_t fadc_rawADC[FADC_NCHAN][MAXRAW];
    Int_t past_hel[6]={0};
    Int_t vtp_hel=0, hel_win_cnt=0;

    T->SetBranchAddress("fadc_rawADC",fadc_rawADC);
    T->SetBranchAddress("vtp_past_hel",past_hel);
    T->SetBranchAddress("vtp_helicity",&vtp_hel);
    T->SetBranchAddress("hel_win_cnt",&hel_win_cnt);

    Int_t nentries = T->GetEntries();
    Int_t Nplus=0, Nminus=0;

    TH1F *hplus = new TH1F("hplus","counts for plus helicity",100,600,700);
    TH1F *hminus = new TH1F("hminus","counts for minus helicity",100,600,700);
    TH1F *hasym = new TH1F("hasym","asymmetry distribution",100,0.0005,0.001);

    bool change=false;
    bool firstcheckhel=true;
    int pre_hel=0;

    int np_tot=0;
    int nm_tot=0;

    int helpos=0;
    int ndiff=0;
    int quadstart=0;
    
    int vtp_pre_hel=0;
    int vtp_pre_win=0; 
    int vtp_cur_win=0; 
    int vtp_cur_hel=0; 
    int Nplus_quad=0, Nminus_quad=0;

    bool isquad=false;
   
    for(int ii=0; ii<nentries; ii++){
	T->GetEntry(ii);

 	struct fadc_pulse pulses[2];
	pulses[0]=FindPulses(fadc_rawADC[0],width,NPED[0]);   // channel 0: asymmetry signals
	pulses[1]=FindPulses(fadc_rawADC[2],width,NPED[2]);   // channel 2: helicity signals 

// find the hel_win_cnt for the first quad, and initialize pre_hel, pre_win
	if(ii==0){
	   quadstart=FindQuad(past_hel, &helpos);
	   helpos=helpos+1;
	   quadstart=hel_win_cnt+(quadstart+1-maxbits);
	   //cout<<"Quad start at  "<<quadstart<<"  "<<hel_win_cnt<<endl;
	   if(quadstart==-1) cout<<"Couldn't find the start of quad"<<endl;
	   //cout<<"Quad start at  "<<helpos<<endl;

	   vtp_pre_hel=vtp_hel;
	   vtp_pre_win=hel_win_cnt;
	   pre_hel=pulses[1].npulse;
	}	

	vtp_cur_win=hel_win_cnt;
	vtp_cur_hel=vtp_hel;

// check if the vtp helicity is the same as fadc helicity; vtp helicity is one window late than the fadc hel
	if(vtp_cur_win!=vtp_pre_win){
	  int win_df=vtp_cur_win-vtp_pre_win;
	  if(win_df>1) cout<<"Something wrong with the hel win counts"<<endl;

	  if(vtp_cur_hel!=pre_hel) ndiff++;
	
	  vtp_pre_hel=vtp_cur_hel;
	  vtp_pre_win=vtp_cur_win;  

// check if the helicity matches the prediction
	  if((vtp_cur_win-quadstart)>120 && firstcheckhel){
	     int tmp=maxbits-(vtp_cur_win-quadstart)-1;
	     bool checkquad=CheckQuad(past_hel, tmp);
	     if(checkquad==false) cout<<"Couldn't match the prediction"<<endl;
	     firstcheckhel=false;
	  }
	  change=true;
	  helpos=(helpos+1)%4;

	  if(helpos==0) {
	     isquad=true;
	     double tmp_asym=0;
	     if((Nplus_quad+Nminus_quad)>0) tmp_asym=1.0*(Nplus_quad-Nminus_quad)/(1.0*(Nplus_quad+Nminus_quad));
	     hasym->Fill(tmp_asym);

	     Nplus_quad=0;
	     Nminus_quad=0;
	  }
	  else isquad=false;
	}

	int cur_hel=pulses[1].npulse;
// fill the counts for pos/neg helicity distributions

	if(change){
	   if(pre_hel==1)hplus->Fill(Nplus);
	   if(pre_hel==0)hminus->Fill(Nminus);
//cout<<"!!  "<<Nplus<<" "<<Nminus<<"   "<<Nplus_quad<<"  "<<Nminus_quad<<"  "<<pre_hel<<"  "<<helpos<<endl;	   
	   Nplus=0;
	   Nminus=0;
	   change=false;
	   pre_hel=cur_hel;
	}

	if(cur_hel==1){
	  Nplus += pulses[0].npulse;
	  Nplus_quad += pulses[0].npulse;
	  np_tot++;
	}	
	if(cur_hel==0){
	  Nminus += pulses[0].npulse;
	  Nminus_quad += pulses[0].npulse;
	  nm_tot++;
	}
	if(cur_hel>1 || pulses[0].npulse>1){
	  cout<<"Let me think what to do with it:  "<<pulses[1].npulse<<"  "<<pulses[0].npulse<<endl;
	}

    }

    cout<<"FADC hel and VTP hel diff   "<<ndiff<<endl;
  
    gStyle->SetOptStat(111111);
    TCanvas *c1=new TCanvas("c1","c1",1500,1500);
    c1->Divide(3,1);
    c1->cd(1);
    hplus->Draw();
    c1->cd(2);
    hminus->Draw();
    c1->cd(3);
    hasym->Draw();
   
    cout<<nm_tot<<"  "<<np_tot<<endl; 
}
