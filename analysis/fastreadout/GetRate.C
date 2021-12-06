Double_t GetRate(int nrun, int chan){

     TChain *T = new TChain("T");
     TString rootfile = Form("/home/daq/work/SOLID_DAQ/decoder/Rootfiles/fadctest_%d*",nrun);
     T->Add(rootfile);
     	
     gROOT->SetBatch(kTRUE);
   
     TH1F *hrate = new TH1F("hrate","rate distribution",25000,0,250000);
     T->Draw(Form("fadc_scal_rate[%d]>>hrate",chan),"fadc_scal_trigcnt!=0");
     Double_t aRate = hrate->GetBinCenter(hrate->GetMaximumBin());

     delete hrate;

     return aRate;

}
