#include "/home/daq/work/SOLID_DAQ/decoder/SetParams.h"
#include "FindPulses.h"

void CalcAsym(){

    int nrun=0;
    cout<<"Which run?  ";
    cin>>nrun;

    TChain *T = new TChain("T");
    T->Add(Form("/home/daq/work/SOLID_DAQ/decoder/Rootfiles/fadctest_%d.root",nrun));


    int NPED[16]={700,0,200,0,0,0,0,0,0,0,0,0,0,0,0,0};
    int width=30; 

    Int_t fadc_scal[16],fadc_a[16],fadc_scal_trigcnt,vtp_trigcnt[5];
    Int_t fadc_rawADC[FADC_NCHAN][MAXRAW];

    T->SetBranchAddress("fadc_rawADC",fadc_rawADC);

    Int_t nentries = T->GetEntries();
    Int_t Nplus=0, Nminus=0;

    for(int ii=0; ii<nentries; ii++){
	T->GetEntry(ii);
	
 	struct fadc_pulse pulses[2];
	pulses[0]=FindPulses(fadc_rawADC[0],width,NPED[0]);   // channel 0: asymmetry signals
	pulses[1]=FindPulses(fadc_rawADC[2],width,NPED[2]);   // channel 2: helicity signals 

	if(pulses[1].npulse==1){
	  Nplus += pulses[0].npulse;
	}	
	if(pulses[1].npulse==0){
	  Nminus += pulses[0].npulse;
	}
	if(pulses[1].npulse>1){
	  cout<<"Let me think what to do with it:  "<<pulses[1].npulse<<endl;
	}
	
    }

    Double_t asym = 1.0*(Nplus-Nminus)/(1.0*(Nplus+Nminus));
    cout<<"+:  "<<Nplus<<endl;
    cout<<"-:  "<<Nminus<<endl;
 
    cout<<asym<<endl;

}
