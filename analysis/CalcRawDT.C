#include "../SetParams.h"
void CalcRawDT(int nrun, Double_t& dt1, Double_t& dt2, Double_t& dt1_err, Double_t& dt2_err){

     TString filename = Form("../Rootfiles/fadctest_%d.root", nrun);

     Int_t NPED=100;

     TFile *f0 = new TFile(filename);
     TTree *T = (TTree*) f0->Get("T");
     TTree *VTP = (TTree*) f0->Get("VTP");
     T->AddFriend(VTP);

     Int_t fadc_scal[16],fadc_a[16],fadc_scal_trigcnt,vtp_trigcnt[5];
     Int_t fadc_rawADC[FADC_NCHAN][MAXRAW];

     T->SetBranchAddress("fadc_scal_cnt",fadc_scal);
     T->SetBranchAddress("fadc_rawADC",fadc_rawADC);
     T->SetBranchAddress("fadc_scal_trigcnt",&fadc_scal_trigcnt);
     T->SetBranchAddress("trigcnt",&vtp_trigcnt);

     Int_t nentries = T->GetEntries();

     Double_t total_true_ch0=0, total_scal_ch0=0, total_scal_trig=0, total_vtp_trig=0;
     for(int ii=0; ii<nentries; ii++){
	T->GetEntry(ii);

	int nsample=0;
        for(int jj=0; jj<MAXRAW; jj++)
	   if( fadc_rawADC[0][jj]>NPED )nsample++;  

	if(nsample>4) total_true_ch0 += 1;

	if(fadc_scal[0]>total_scal_ch0) total_scal_ch0 =fadc_scal[0];
	if(fadc_scal_trigcnt>total_scal_trig) total_scal_trig = fadc_scal_trigcnt; 
	if(vtp_trigcnt[2]>0) total_vtp_trig += vtp_trigcnt[2]; 
     }

     dt1 = total_true_ch0/total_scal_ch0;
     dt2 = total_scal_trig/total_scal_ch0;

     dt1_err = dt1 * sqrt(1./total_true_ch0 - 1./total_scal_ch0);
     dt2_err = dt2 * sqrt(1./total_scal_trig - 1./total_scal_ch0);

/*
     cout<<"-------------------"<<nrun<<"------------------------"<<endl;
     cout<<"ch0      scal_ch0      scal_trigcnt       vtp_trigcnt"<<endl;
     cout<<total_true_ch0<<"  "<<total_scal_ch0<<"  "<<total_scal_trig<<"  "<<total_vtp_trig<<endl;
     cout<<"dt1      dt2"<<endl;
     cout<<dt1<<"    "<<dt2<<endl;
     cout<<"-----------------------------------------------------"<<endl;
*/


}
