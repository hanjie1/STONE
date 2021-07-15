#include "/home/daq/work/SOLID_DAQ/decoder/SetParams.h"
#include "FindHelicity.h"

void CalcAsym_int(){

  TH1F *hplus = new TH1F("hplus","counts for plus helicity",100,620,720);
  TH1F *hminus = new TH1F("hminus","counts for minus helicity",100,620,720);
  TH1F *hasym = new TH1F("hasym","asymmetry distribution",200,-0.001,0.002);

  TH1F *hplus_s = new TH1F("hplus_s","scaler counts for plus helicity",100,620,720);
  TH1F *hminus_s = new TH1F("hminus_s","scaler counts for minus helicity",100,620,720);
  TH1F *hasym_s = new TH1F("hasym_s","scaler asymmetry distribution",200,-0.001,0.002);

  TH1F *hplus_dt = new TH1F("hplus_dt","plus helicity dead time",500,-0.01,0.01);
  TH1F *hminus_dt = new TH1F("hminus_dt","minus helicity dead time",500,-0.01,0.01);

  TH1F *hasym_diff = new TH1F("hasym_diff","asymmetry difference between scaler and fadc",500,-0.01,0.01);

  int NPED[16]={180,190,190,0,0,0,0,0,0,0,0,0,0,0,0,0};
  int width=13; 

  const int nn_run=1;
  int runlist[1]={484};

  for(int nn=0; nn<nn_run; nn++){

    int nrun=runlist[nn];
//    cout<<"Which run?  ";
//    cin>>nrun;

    TString filename = Form("/home/daq/work/SOLID_DAQ/decoder/Rootfiles/fadctest_%d.root", nrun);

    TFile *f0 = new TFile(filename);
    TTree *T = (TTree*) f0->Get("T");
    TTree *VTP = (TTree*) f0->Get("VTP");
    T->AddFriend(VTP);

    Int_t fadc_scalcnt[FADC_NCHAN];
    Int_t fadc_a[FADC_NCHAN];
    Int_t fadc_a1[FADC_NCHAN];
    Int_t past_hel[6]={0};
    Int_t vtp_hel=0, hel_win_cnt=0,hel_win_cnt_1=0;
    Int_t last_mps_time=0;

    T->SetBranchAddress("fadc_a",fadc_a);
    T->SetBranchAddress("fadc_a1",fadc_a1);
    T->SetBranchAddress("vtp_past_hel",past_hel);
    T->SetBranchAddress("vtp_helicity",&vtp_hel);
    T->SetBranchAddress("hel_win_cnt",&hel_win_cnt);
    T->SetBranchAddress("hel_win_cnt_1",&hel_win_cnt_1);
    T->SetBranchAddress("vtp_fadc_scalcnt",fadc_scalcnt);
    T->SetBranchAddress("last_mps_time",&last_mps_time);

    Int_t nentries = T->GetEntries();
    Int_t Nplus=0, Nminus=0;

    bool change=false;
    bool firstcheckhel=true;
    int fadc_pre_hel=0;
    int fadc_cur_hel=0;

    int np_tot=0;
    int nm_tot=0;

    int helpos=0;
    int ndiff=0;
    int quadstart=0;
    
    int vtp_pre_win=0; 
    int vtp_cur_win=0; 
    int vtp_cur_hel=0; 
    int Nplus_quad=0, Nminus_quad=0;
    int Nplus_quad_s=0, Nminus_quad_s=0;  //quad scaler counts
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

    int nnwin=0;
    bool scaler_updated=true;
    //int nmps=0;
    for(int ii=0; ii<nentries; ii++){
	T->GetEntry(ii);

	if(ii==0) vtp_pre_win=firstquad_hel_win;

	if(hel_win_cnt<firstquad_hel_win) continue;
	
	if(hel_win_cnt!=vtp_pre_win && scaler_updated){
	   pre_win_Nhel=Nhel;
	   Nhel=0;
	   fadc_pre_hel=fadc_cur_hel;
	   scaler_updated=false;
	}

	int npulse=0;
	if(fadc_a[0]>20000) npulse++;
	if(fadc_a1[0]>3000){ npulse++; cout<<"Second pulses found in channel 1"<<endl;}

	if((fadc_a[1]<3000) && last_mps_time>16000) Nhel = Nhel+npulse;

	if((fadc_a[1]<3000) && last_mps_time<300) pre_win_Nhel+=npulse;

	if(fadc_a[1]<3000 && fadc_a[2]<3000) fadc_cur_hel=0;
	if(fadc_a[1]<3000 && fadc_a[2]>40000) fadc_cur_hel=1;

	if(vtp_pre_win==hel_win_cnt_1){  // get scaler counts for the previous hel_win_cnt
	   if(vtp_hel!=fadc_pre_hel) ndiff++;

	   if(vtp_hel==1){
	     hplus->Fill(pre_win_Nhel);		// fadc plus counts
	     hplus_s->Fill(fadc_scalcnt[0]);	// scaler plus counts
	    
	     Double_t p_dt=1-1.0*pre_win_Nhel/(1.0*fadc_scalcnt[0]); // plus window dead time
	     hplus_dt->Fill(p_dt);

	     Nplus_quad=Nplus_quad+pre_win_Nhel;     // fadc plus counts for a quad
	     Nplus_quad_s=Nplus_quad_s+fadc_scalcnt[0];  //scaler plus counts for a quad
//cout<<hel_win_cnt_1<<"  "<<pre_win_Nhel<<"  "<<fadc_scalcnt[0]<<endl;
	   }	   

	   if(vtp_hel==0){
	     hminus->Fill(pre_win_Nhel);		// fadc minus counts
	     hminus_s->Fill(fadc_scalcnt[0]);	// scaler minus counts
	    
	     Double_t m_dt=1-1.0*pre_win_Nhel/(fadc_scalcnt[0]*1.0); // minus window dead time
	     hminus_dt->Fill(m_dt);

	     Nminus_quad=Nminus_quad+pre_win_Nhel;     // fadc minus counts for a quad
	     Nminus_quad_s=Nminus_quad_s+fadc_scalcnt[0];  //scaler minus counts for a quad
	   }	   

	   nnwin++;

	   FindQuad(past_hel,&helpos);
	   helpos=helpos%4;   // helicity window of the past helicity
	   if(helpos==3 && nnwin==4){
	     Double_t tmp_asym=1.0*(Nplus_quad-Nminus_quad)/(1.0*Nplus_quad+Nminus_quad);
	     hasym->Fill(tmp_asym); 

             Double_t tmp_asym_s=1.0*(Nplus_quad_s-Nminus_quad_s)/(1.0*Nplus_quad_s+Nminus_quad_s);
	     hasym_s->Fill(tmp_asym_s);

	     Double_t asym_diff = (tmp_asym-tmp_asym_s);
	     hasym_diff->Fill(asym_diff);
//if(asym_diff>0.1) cout<<hel_win_cnt_1<<"  "<<asym_diff<<"  "<<Nplus_quad<<"  "<<Nminus_quad<<"  "<<Nplus_quad_s<<"  "<<Nminus_quad_s<<endl;

	     nnwin=0; 
	     Nplus_quad=0; Nminus_quad=0;
	     Nplus_quad_s=0; Nminus_quad_s=0;
	   }  

	   vtp_pre_win=hel_win_cnt;
	   scaler_updated=true;
	}		
    }

    delete T;
    cout<<"Run "<<nrun<<":  FADC hel and VTP hel diff   "<<ndiff<<endl;
  }
  
    gStyle->SetOptStat(111111);
    TCanvas *c1=new TCanvas("c1","c1",1500,1500);
    c1->Divide(2,3);
    c1->cd(1);
    hplus->Draw();
    c1->cd(2);
    hminus->Draw();
    c1->cd(3);
    hplus_s->Draw();
    c1->cd(4);
    hminus_s->Draw();
    c1->cd(5);
    hplus_dt->Draw();
    c1->cd(6);
    hminus_dt->Draw();

    TCanvas *c2=new TCanvas("c2","c2",1500,1500);
    c2->Divide(3,1);
    c2->cd(1);
    hasym->Draw();
    c2->cd(2);
    hasym_s->Draw();
    c2->cd(3);
    hasym_diff->Draw();
      
    //cout<<"MPS:  "<<nmps<<endl; 
}
