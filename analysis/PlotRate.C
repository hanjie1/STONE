void PlotRate(int nrun){
     TString filename = Form("../Rootfiles/fadctest_%d.root", nrun);

     TFile *f0 = new TFile(filename);
     TTree *T = (TTree*) f0->Get("T");

     Int_t fadc_scal_cnt[16],fadc_scal_trigcnt,fadc_scal_time;

     T->SetBranchAddress("fadc_scal_cnt",fadc_scal_cnt);
     T->SetBranchAddress("fadc_scal_time",&fadc_scal_time);
     T->SetBranchAddress("fadc_scal_trigcnt",&fadc_scal_trigcnt);

     Int_t nentries = T->GetEntries();

     Int_t prev_cnt[16]={0};
     Int_t prev_time=0, prev_trigcnt=0;
     Int_t nskip=500;

     TGraph *gRate0 = new TGraph();
     TGraph *gRate1 = new TGraph();
     //TH1F *hrate[2];
     //hrate[0] = new TH1F("hrate_0","fadc chan0 rate distribution",200,0,);
     int npoint = 0;
     for(int ii=0; ii<nentries;ii++){
	T->GetEntry(ii);
	if(fadc_scal_trigcnt==0) continue;

	Int_t delta_n = fadc_scal_trigcnt-prev_trigcnt; 

 	if(delta_n>nskip){
	   Double_t delta_T = (fadc_scal_time - prev_time)*2048.0*1e-9;  // s
	   if(delta_T==0) cout<<"Something with the time !"<<endl;
	   Double_t rate0 = (fadc_scal_cnt[0]-prev_cnt[0])*1.0/delta_T;
	   Double_t rate1 = (fadc_scal_cnt[1]-prev_cnt[1])*1.0/delta_T;
	
	   gRate0->SetPoint(npoint,npoint,rate0);
	   gRate1->SetPoint(npoint,npoint,rate1);
	   npoint++;

	   prev_trigcnt = fadc_scal_trigcnt;
	   prev_time = fadc_scal_time;
	   for(int nn=0; nn<16; nn++)
	      prev_cnt[nn] = fadc_scal_cnt[nn];
	}

     }

     TCanvas *c1 = new TCanvas("c1","c1",1000,1500);
     c1->Divide(2,1);
     c1->cd(1);
     gRate0->SetMarkerStyle(8);
     gRate0->Draw("AP");
     gRate0->SetTitle("Chan0 rate;entry;rate;");

     c1->cd(2);
     gRate1->SetMarkerStyle(8);
     gRate1->Draw("AP");
     gRate1->SetTitle("Chan1 rate;entry;rate;");

}
