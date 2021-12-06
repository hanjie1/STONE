#include "../../SetParams.h"
void CalcRawDT(int nrun, int chan, Double_t& dt1, Double_t& dt1_err){

     //TString filename = Form("../../Rootfiles/fastreadout_%d.root", nrun);
     TString filename = Form("../../Rootfiles/fadctest_%d.root", nrun);

     Int_t NPED[16]={101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101};

     TFile *f0 = new TFile(filename);
     TTree *T = (TTree*) f0->Get("T");

     Int_t fadc_scal_cnt[16],fadc_scal_rate[16];
     Int_t fadc_rawADC[FADC_NCHAN][MAXRAW];

     T->SetBranchAddress("fadc_scal_cnt",fadc_scal_cnt);
     T->SetBranchAddress("fadc_rawADC",fadc_rawADC);
     T->SetBranchAddress("fadc_scal_rate",&fadc_scal_rate);

     Int_t nentries = T->GetEntries();

     Double_t total_true_ch0=0, total_scal_ch0=0;
     Double_t last_count=0,first_scal_ch0=0;
     bool first=true;
     bool isUpdate=false;

     for(int ii=0; ii<nentries; ii++){
	T->GetEntry(ii);

	if(first && fadc_scal_cnt[0]>0) {
		first=false;
		first_scal_ch0=fadc_scal_cnt[0];
	}	
	if(first) continue;


	if(fadc_scal_cnt[0]>0){
	   isUpdate=true;
	   total_true_ch0+=last_count;
	   last_count=0;
	   total_scal_ch0=fadc_scal_cnt[0]-first_scal_ch0;
	}
	else
	   isUpdate=false;

	int nsample=0;
        for(int jj=0; jj<MAXRAW; jj++){
	   if( fadc_rawADC[chan][jj]>NPED[chan] )nsample++;  
	   else{
		if(nsample>4) last_count+=1;
		nsample=0;
	   }  
	}

	if(nsample>4) last_count+=1;

     }

     dt1 = total_true_ch0/total_scal_ch0;
     dt1_err = dt1 * sqrt(1./total_true_ch0 - 1./total_scal_ch0);


     cout<<"-------------------"<<nrun<<"------------------------"<<endl;
     cout<<"ch0      scal_ch0         dt            dt_err"<<endl;
     cout<<total_true_ch0<<"  "<<total_scal_ch0<<"  "<<dt1<<"  "<<dt1_err<<endl;
     cout<<"-----------------------------------------------------"<<endl;



}
