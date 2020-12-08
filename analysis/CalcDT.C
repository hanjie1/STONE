void CalcDT(int nrun, Double_t& dt1, Double_t& dt2){

     TString filename = Form("../Rootfiles/fadctest_%d.root", nrun);

     TFile *f0 = new TFile(filename);
     TTree *T = (TTree*) f0->Get("T");
     TTree *VTP = (TTree*) f0->Get("VTP");
     T->AddFriend(VTP);

     Int_t fadc_scal[16],fadc_a[16],fadc_scal_trigcnt,vtp_trigcnt[5];
     Int_t busytime, livetime;

     T->SetBranchAddress("fadc_scal_cnt",fadc_scal);
     T->SetBranchAddress("fadc_a",&fadc_a);
     T->SetBranchAddress("fadc_scal_trigcnt",&fadc_scal_trigcnt);
     T->SetBranchAddress("trigcnt",&vtp_trigcnt);
//     T->SetBranchAddress("busytime",&busytime);
//     T->SetBranchAddress("livetime",&livetime);

     Int_t nentries = T->GetEntries();

     Double_t total_true_ch0=0, total_scal_ch0=0, total_scal_trig=0, total_vtp_trig=0;
     for(int ii=0; ii<nentries; ii++){
	T->GetEntry(ii);

	if(fadc_a[0]>0) total_true_ch0 += 1;
	if(fadc_scal[0]>total_scal_ch0) total_scal_ch0 =fadc_scal[0];
	if(fadc_scal_trigcnt>total_scal_trig) total_scal_trig = fadc_scal_trigcnt; 
	if(vtp_trigcnt[2]>0) total_vtp_trig += vtp_trigcnt[2]; 
     }

     dt1 = total_true_ch0/total_scal_ch0;
     dt2 = total_scal_trig/total_scal_ch0;
/*
     cout<<"-------------------"<<nrun<<"------------------------"<<endl;
     cout<<"ch0      scal_ch0      scal_trigcnt       vtp_trigcnt"<<endl;
     cout<<total_true_ch0<<"  "<<total_scal_ch0<<"  "<<total_scal_trig<<"  "<<total_vtp_trig<<endl;
     cout<<"dt1      dt2"<<endl;
     cout<<dt1<<"    "<<dt2<<endl;
     cout<<"-----------------------------------------------------"<<endl;
*/


}
