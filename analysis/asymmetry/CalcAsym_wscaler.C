#include "/home/daq/work/SOLID_DAQ/decoder/SetParams.h"
#include "FindHelicity.h"
#include "FindPulses.h"

void CalcAsym_wscaler(){

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

    Int_t fadc_scal[FADC_NCHAN];
    Int_t fadc_rawADC[FADC_NCHAN][MAXRAW];
    Int_t past_hel[6]={0};
    Int_t vtp_hel=0, hel_win_cnt=0,hel_win_cnt_1=0;

    T->SetBranchAddress("fadc_rawADC",fadc_rawADC);
    T->SetBranchAddress("vtp_past_hel",past_hel);
    T->SetBranchAddress("vtp_helicity",&vtp_hel);
    T->SetBranchAddress("hel_win_cnt",&hel_win_cnt);
    T->SetBranchAddress("hel_win_cnt_1",&hel_win_cnt_1);
    T->SetBranchAddress("vtp_fadc_scalcnt",fadc_scal);

    Int_t nentries = T->GetEntries();
    Int_t Nplus=0, Nminus=0;

    TH1F *hplus = new TH1F("hplus","counts for plus helicity",100,600,700);
    TH1F *hminus = new TH1F("hminus","counts for minus helicity",100,600,700);
    TH1F *hasym = new TH1F("hasym","asymmetry distribution",200,-0.1,0.1);

    TH1F *hplus_s = new TH1F("hplus_s","scaler counts for plus helicity",100,600,700);
    TH1F *hminus_s = new TH1F("hminus_s","scaler counts for minus helicity",100,600,700);
    TH1F *hasym_s = new TH1F("hasym_s","scaler asymmetry distribution",200,-0.1,0.1);

    TH1F *hplus_dt = new TH1F("hplus_dt","plus helicity dead time",100,0,1);
    TH1F *hminus_dt = new TH1F("hminus_dt","minus helicity dead time",100,0,1);
    TH1F *htotal_dt = new TH1F("htotal_dt","total helicity dead time",100,0,1);

    bool change=false;
    bool firstcheckhel=true;
    int fadc_pre_hel=0;
    int fadc_cur_hel=0;

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
    int Nhel=0;   // counts per helicity window
    int pre_win_Nhel=0;   // counts per helicity window

    bool isQuadStart=false;
    bool isFindQuad=false;
    int firstquad_hel_win=0;  // the hel_win_cnt where the first quad starts

    for(int ii=0; ii<nentries; ii++){   // find the helicity window count for the first quad 
	T->GetEntry(ii);

	if(isFindQuad==false){
	  quadstart=FindQuad(past_hel,&helpos);
	  helpos=(helpos+1)%4;
	  if(quadstart==-1) {
		cout<<"Couldn't find the start of quad!"<<endl;
		break;
	  }
	 
	  isFindQuad=true; 

	  vtp_pre_win=hel_win_cnt;
	  if(helpos!=0) continue;
	}

	if(hel_win_cnt!=vtp_pre_win){
	   helpos=(helpos+1)%4;
	   vtp_pre_win=hel_win_cnt;
	}
	if(helpos!=0) isQuadStart=false;
        else isQuadStart=true; 

	if(isQuadStart){
	  firstquad_hel_win=hel_win_cnt;
	  break;
	}
    }

    cout<<"First quad wind: "<<firstquad_hel_win<<endl;

    for(int ii=0; ii<nentries; ii++){
	T->GetEntry(ii);

	if(ii==0) vtp_pre_win=firstquad_hel_win;

	if(hel_win_cnt<firstquad_hel_win) continue;
	
	if(hel_win_cnt!=vtp_pre_hel){
	   pre_win_Nhel=Nhel;
	   Nhel=0;
	   fadc_pre_hel=fadc_cur_hel;
	}

        struct fadc_pulse pulses[2];
        pulses[0]=FindPulses(fadc_rawADC[0],width,NPED[0]);   // channel 0: asymmetry signals
        pulses[1]=FindPulses(fadc_rawADC[2],width,NPED[2]);   // channel 2: helicity signals 
	
	Nhel = Nhel+pulses[0].npulse;

	fadc_cur_hel=pulses[1].npulse;

	if(pulses[0].npulse>1 || pulses[1].npulse>1) cout<<"More than 1 pulses are found in one fadc window:  "<<pulses[0].npulse<<"  "<<pulses[1].npulse<<endl;
	
	if(vtp_pre_hel==hel_win_cnt_1){  // get scaler counts for the previous hel_win_cnt
	   if(vtp_helicity!=fadc_pre_hel) ndiff++;

	   if(vtp_helicity==1){
	     hplus->Fill(pre_win_Nhel);		
	     Nplus=Nplus+pre_win_Nhel;
	     
	   }	   

	   if(vtp_helicity==1){
	     hplus->Fill(pre_win_Nhel);		
	     Nplus=Nplus+pre_win_Nhel;
	   }	   
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
