void TriggerTime(){
    int nrun=0;
    cout<<"Which run?  ";
    cin>>nrun;

    TString filename = Form("/home/daq/work/SOLID_DAQ/decoder/Rootfiles/fadctest_%d.root", nrun);

    TFile *f0 = new TFile(filename);
    TTree *T = (TTree*) f0->Get("T");
    TTree *VTP = (TTree*) f0->Get("VTP");
    T->AddFriend(VTP);

    ULong64_t fadc_trigtime=0, pre_fadc_trigtime=0;
    ULong64_t vtp_trigtime=0, pre_vtp_trigtime=0;
    ULong64_t ti_timestamp=0,pre_ti_timestamp=0;

    T->SetBranchAddress("fadc_trigtime",&fadc_trigtime);
    T->SetBranchAddress("vtp_trigtime",&vtp_trigtime);
    T->SetBranchAddress("ti_timestamp",&ti_timestamp);

    Int_t nentries = T->GetEntries();

    TH1F *hfadc=new TH1F("hfadc","fadc trigger time difference",500,0,4e9);
    TH1F *hvtp=new TH1F("hvtp","vtp trigger time difference",500,0,4e4);
    TH1F *hti=new TH1F("hti","ti time stamp difference",300,0,3e4);

    for(int ii=0; ii<nentries; ii++){
	T->GetEntry(ii);

	ULong64_t fadc_diff=fadc_trigtime-pre_fadc_trigtime;
	hfadc->Fill(fadc_diff);

	ULong64_t vtp_diff=vtp_trigtime-pre_vtp_trigtime;
	hvtp->Fill(vtp_diff);

	ULong64_t ti_diff=ti_timestamp-pre_ti_timestamp;
	hti->Fill(ti_diff);

	pre_fadc_trigtime=fadc_trigtime;
	pre_vtp_trigtime=vtp_trigtime;
	pre_ti_timestamp=ti_timestamp;
    } 

    gStyle->SetOptStat(111111);

    TCanvas *c1=new TCanvas("c1","c1",1500,1500);
    c1->Divide(3,1);
    c1->cd(1);
    hfadc->Draw();
    c1->cd(2);
    hvtp->Draw();
    c1->cd(3);
    hti->Draw();

}
