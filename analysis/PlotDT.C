#include "CalcDT.C"
#include "GetRate.C"

void PlotDT(){
     const int np=10; 
     int run_number[np]={159,160,161,186,163,165,166,187,193,204};
     int nn[np] = {14,8,7,9,6,5,4,3,3,3};

     TGraph *gDT1 = new TGraph();
     TGraph *gDT2 = new TGraph();

     for(int ii=0; ii<np; ii++){
	Double_t dt1=0, dt2=0;
	CalcDT(run_number[ii],dt1,dt2);

	gDT1->SetPoint(ii,nn[ii],1-dt1);
	gDT2->SetPoint(ii,nn[ii],1-dt2);
        //cout<<dt1<<"  "<<dt2<<endl;
     }

     TGraph *gRate_ch0 = new TGraph();
     TGraph *gRate_ch0_n = new TGraph(); // rate calculated as 460 kHz/2^(n-1)

     for(int ii=0; ii<np; ii++){
	Double_t thisRate0 = GetRate(run_number[ii],0);
	gRate_ch0->SetPoint(ii,nn[ii],thisRate0);	

	Double_t thisRate_th = 460000.0/pow(2,nn[ii]-1);
	gRate_ch0_n->SetPoint(ii,nn[ii],thisRate_th);	
     }

     gROOT->SetBatch(kFALSE);
     TCanvas *c3 = new TCanvas("c3","c3",1000,1000);
     gDT1->SetMarkerStyle(8);
     gDT1->SetMarkerColor(4);

     gDT2->SetMarkerStyle(22);
     gDT2->SetMarkerColor(2);

     TMultiGraph *mg = new TMultiGraph();
     mg->Add(gDT1);
     mg->Add(gDT2);
     mg->Draw("AP");

     TLegend *leg = new TLegend(0.7,0.7,0.85,0.85);
     leg->AddEntry(gDT1,"method 1","P");
     leg->AddEntry(gDT2,"method 2","P");
     leg->Draw();

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

