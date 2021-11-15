 #include "TDirectory.h"
 #include "SetParams.h"


void PlotRawMode(){
     int runnumber=0;
     cout<<"Which run ?  ";
     cin>>runnumber;

     int nstart=0;
     cout<<"Which event to start?  ";
     cin>>nstart;

     TString filename = Form("Rootfiles/fastreadout_%d.root", runnumber); 

     TFile *f0 = new TFile(filename);
     if(! (f0->IsOpen()) ) return;
     TTree *T = (TTree*) f0->Get("T");

     TString outfile = Form("Rootfiles_rawmode/fastreadout_mode1_%d.root", runnumber);
     TFile *f1 = new TFile(outfile,"RECREATE","fadc raw mode plots");

     int nsnap = 1;
     cout<<"How many number of snaps ? ";
     cin>>nsnap;

     TDirectory *raw = f1->mkdir("mode1");;	   
     raw->cd();
     TDirectory *rawchan[FADC_NCHAN];
     TGraph *hsnap[FADC_NCHAN][nsnap];

     for(int ii=0;ii<FADC_NCHAN;ii++){
	   TString tmpdir=Form("chan_%i",ii);
	   rawchan[ii] = raw->mkdir(tmpdir);
	   rawchan[ii]->cd();
	   for(int jj=0;jj<nsnap;jj++){
		hsnap[ii][jj] = new TGraph(MAXRAW);
		hsnap[ii][jj]->SetName(Form("ch%i_event%i",ii,jj));
	   }
       raw->cd();
     }

     Int_t frawdata[FADC_NCHAN][MAXRAW];
     memset(frawdata, 0, FADC_NCHAN*MAXRAW*sizeof(frawdata[0][0]));

     T->SetBranchAddress("fadc_rawADC",frawdata);

     Int_t nentries = T->GetEntries();
     if(nentries<nsnap){
	cout<<"Warning: Not that many events ! "<<endl;
	nsnap = nentries;		
     }

     for(int ii=nstart;ii<nstart+nsnap;ii++){
	T->GetEntry(ii);
	for(int jj=0;jj<FADC_NCHAN;jj++){
	  rawchan[jj]->cd();
	  for(int kk=0; kk<MAXRAW; kk++){
	     hsnap[jj][ii-nstart]->SetPoint(kk,kk,frawdata[jj][kk]);
	  }
	  hsnap[jj][ii-nstart]->Write();
	}
     }
    f1->Write();
    delete f1;

}
