#include "/home/daq/work/SOLID_DAQ/decoder/SetParams.h"
#include "/home/daq/work/SOLID_DAQ/decoder/FindHelicity.h"
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

    TH1F *hplus = new TH1F("hplus","counts for plus helicity",20,0,20);
    TH1F *hminus = new TH1F("hminus","counts for minus helicity",20,0,20);
    TH1F *hasym = new TH1F("hasym","asymmetry distribution",100,-1,1);

    bool change=false;
    int pre_hel=0;

    int np_tot=0;
    int nm_tot=0;

    int helpos=0;
    int ndiff=0;
    
    int vtp_pre_hel=0;
    int vtp_pre_win=0; 
    int vtp_cur_win=0; 
    int vtp_cur_hel=0; 

    bool findquad=false;
    for(int ii=0; ii<nentries; ii++){
	T->GetEntry(ii);

	if(ii==0){
	   findquad=FindQuad(past_hel, &helpos);
	   if(findquad) cout<<"found the start of quad at: "<<hel_win_cnt<<endl;
	   cout<<"start quad:  "<<helpos<<endl;

	   vtp_pre_hel=vtp_hel;
	   vtp_pre_win=hel_win_cnt;
	}	


	vtp_cur_win=hel_win_cnt;
	vtp_cur_hel=vtp_hel;

	if(vtp_cur_win!=vtp_pre_win){
	  int win_df=vtp_cur_win-vtp_pre_win;
	  if(win_df>1) cout<<"Something wrong with the hel win counts"<<endl;

	  if(vtp_cur_hel!=pre_hel) ndiff++;
	
	  vtp_pre_hel=vtp_cur_hel;
	  vtp_pre_win=vtp_cur_win;  

	  if(findquad==false){
	     findquad=FindQuad(past_hel, &helpos);
	     if(findquad) cout<<"found the start of quad at: "<<hel_win_cnt<<endl;
	  }
	}


 	struct fadc_pulse pulses[2];
	pulses[0]=FindPulses(fadc_rawADC[0],width,NPED[0]);   // channel 0: asymmetry signals
	pulses[1]=FindPulses(fadc_rawADC[2],width,NPED[2]);   // channel 2: helicity signals 

	if(ii==0)   pre_hel=pulses[1].npulse;
	int cur_hel=pulses[1].npulse;

	if(cur_hel!= pre_hel) change=true;

	if(change){
	   if(pre_hel==1)hplus->Fill(Nplus);
	   if(pre_hel==0)hminus->Fill(Nminus);
	   
	   Nplus=0;
	   Nminus=0;
	   change=false;
	   pre_hel=cur_hel;
	}

	if(cur_hel==1){
	  Nplus += pulses[0].npulse;
	  np_tot++;
	}	
	if(cur_hel==0){
	  Nminus += pulses[0].npulse;
	  nm_tot++;
	}
	if(cur_hel>1 || pulses[0].npulse>1){
	  cout<<"Let me think what to do with it:  "<<pulses[1].npulse<<"  "<<pulses[0].npulse<<endl;
	}

    }

    cout<<"FADC hel and VTP hel diff   "<<ndiff<<endl;
  
    gStyle->SetOptStat(111111);
    TCanvas *c1=new TCanvas("c1","c1",1500,1500);
    c1->Divide(2,1);
    c1->cd(1);
    hplus->Draw();
    c1->cd(2);
    hminus->Draw();
   
    cout<<nm_tot<<"  "<<np_tot<<endl; 
}
