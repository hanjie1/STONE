#include "CalcRawDT.C"
#include "GetRate.C"

void PlotDT_rawmode(){
     const int np=7; 
     
     int run_number[np]={337,336,335,334,333,332,328};
     //int run_number[np]={99,100,101,102,104,105,106,107};
     int nn[np] = {9,8,7,6,5,4,3};

     TGraphErrors *gDT1 = new TGraphErrors();
     TGraph *gRate_ch0 = new TGraph();
     TGraph *gRate_ch0_n = new TGraph(); // rate calculated as 460 kHz/2^(n-1)

     ofstream outfile1;
     outfile1.open("deadtime_rawmode.txt",ios::app);
     for(int ii=0; ii<np; ii++){
	Double_t dt1=0;
	Double_t dt1_err=0;
	CalcRawDT(run_number[ii],0,dt1,dt1_err);

	gDT1->SetPoint(ii,nn[ii],1-dt1);
	gDT1->SetPointError(ii,0,dt1_err);

	Double_t thisRate0 = GetRate(run_number[ii],0);
	gRate_ch0->SetPoint(ii,nn[ii],thisRate0);	

	Double_t thisRate_th = 460000.0/pow(2,nn[ii]-1);
	gRate_ch0_n->SetPoint(ii,nn[ii],thisRate_th);	

        outfile1<<thisRate0<<"  "<<dt1<<"  "<<dt1_err<<endl;

     }

     outfile1.close();

     gROOT->SetBatch(kFALSE);
     TCanvas *c3 = new TCanvas("c3","c3",1000,1000);
     gDT1->SetMarkerStyle(8);
     gDT1->SetMarkerColor(4);
     gDT1->Draw("AP");

     TCanvas *c2 = new TCanvas("c2","c2",1000,1000);
     gRate_ch0->SetMarkerStyle(8);
     gRate_ch0->SetMarkerColor(2);
     gRate_ch0_n->SetMarkerStyle(22);
     gRate_ch0_n->SetMarkerColor(4);

     TMultiGraph *mg1 = new TMultiGraph();
     mg1->Add(gRate_ch0);
     mg1->Add(gRate_ch0_n);
     mg1->Draw("AP");

     TLegend *leg1 = new TLegend(0.7,0.7,0.85,0.85);
     leg1->AddEntry(gRate_ch0,"measured rate","P");
     leg1->AddEntry(gRate_ch0_n,"expected rate","P");
     leg1->Draw();

}

