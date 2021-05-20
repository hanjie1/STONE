#include "CalcDT.C"
#include "GetRate.C"

void PlotMultiRunRate(){
     const int np=9; 
     const int np1=4;
/*
     int run_number[np]={236,238,240,241,242,244,245,246,247,249};
     int nn[np]={1,10,5,15,20,25,30,35,32,38};  // block level

     int run1[np1]={245,250,251,252,253,254,256};
     int nn1[np1]={1,5,10,15,20,25,30}; // buffer level

     int run_number[np]={286,287,289,290,292};
     int nn[np]={30,10,20,35,40};  // block level

     int run1[np1]={290,293,294};
     int nn1[np1]={1,5,10}; // buffer level
*/

/* mode 4 new pulse */
     int run_number[np]={314,319,320,321,322,323,324,325,326};
     int nn[np]={10,15,20,25,30,35,40,45,50};  // block level

     int run1[np1]={327,328,329,330};
     int nn1[np1]={5,10,15,20}; // buffer level

     TGraphErrors *gDT1 = new TGraphErrors();
     TGraphErrors *gDT2 = new TGraphErrors();

     for(int ii=0; ii<np; ii++){
	Double_t dt1=0, dt2=0;
	Double_t dt1_err=0, dt2_err=0;
	CalcDT(run_number[ii],dt1,dt2,dt1_err,dt2_err);

	gDT1->SetPoint(ii,nn[ii],1-dt1);
	gDT1->SetPointError(ii,0,dt1_err);
	gDT2->SetPoint(ii,nn[ii],1-dt2);
	gDT2->SetPointError(ii,0,dt2_err);
        //cout<<dt1<<"  "<<dt1_err<<"   "<<dt2<<"  "<<dt2_err<<endl;
     }

     TGraphErrors *gDT3 = new TGraphErrors();
     TGraphErrors *gDT4 = new TGraphErrors();

     for(int ii=0; ii<np1; ii++){
	Double_t dt1=0, dt2=0;
	Double_t dt1_err=0, dt2_err=0;
	CalcDT(run1[ii],dt1,dt2,dt1_err,dt2_err);

	gDT3->SetPoint(ii,nn1[ii],1-dt1);
	gDT3->SetPointError(ii,0,dt1_err);
	gDT4->SetPoint(ii,nn1[ii],1-dt2);
	gDT4->SetPointError(ii,0,dt2_err);
        //cout<<dt1<<"  "<<dt1_err<<"   "<<dt2<<"  "<<dt2_err<<endl;
     }

     TGraph *gRate_ch0 = new TGraph();
     TGraph *gRate_ch0_1 = new TGraph();

     for(int ii=0; ii<np; ii++){
	Double_t thisRate0 = GetRate(run_number[ii],0);
	gRate_ch0->SetPoint(ii,nn[ii],thisRate0);	
     }

     for(int ii=0; ii<np1; ii++){
	Double_t thisRate0 = GetRate(run1[ii],0);
	gRate_ch0_1->SetPoint(ii,nn1[ii],thisRate0);	
     }

     gROOT->SetBatch(kFALSE);
     TCanvas *c1 = new TCanvas("c1","c1",1000,1000);
     gDT1->SetMarkerStyle(8);
     gDT1->SetMarkerColor(4);

     gDT2->SetMarkerStyle(22);
     gDT2->SetMarkerColor(2);

     TMultiGraph *mg = new TMultiGraph();
//     mg->Add(gDT1);
     mg->Add(gDT2);
     mg->Draw("AP");
     mg->SetTitle(";blocklevel;deadtime;");

     TLegend *leg = new TLegend(0.7,0.7,0.85,0.85);
//     leg->AddEntry(gDT1,"method 1","P");
     leg->AddEntry(gDT2,"method 2","P");
     leg->Draw();

     TCanvas *c2 = new TCanvas("c2","c2",1000,1000);
     gRate_ch0->SetMarkerStyle(8);
     gRate_ch0->SetMarkerColor(2);

     TMultiGraph *mg1 = new TMultiGraph();
     mg1->Add(gRate_ch0);
     //mg1->Add(gRate_ch0_n);
     mg1->Draw("AP");
     mg1->SetTitle(";blocklevel;rate;");

     TLegend *leg1 = new TLegend(0.7,0.7,0.85,0.85);
     leg1->AddEntry(gRate_ch0,"measured rate","P");
     //leg1->AddEntry(gRate_ch0_n,"expected rate","P");
     leg1->Draw();

     TCanvas *c3 = new TCanvas("c3","c3",1000,1000);
     gDT3->SetMarkerStyle(8);
     gDT3->SetMarkerColor(4);

     gDT4->SetMarkerStyle(22);
     gDT4->SetMarkerColor(2);

     TMultiGraph *mg2 = new TMultiGraph();
     mg2->Add(gDT3);
     mg2->Add(gDT4);
     mg2->Draw("AP");
     mg2->SetTitle(";bufferlevel;deadtime;");

     TLegend *leg2 = new TLegend(0.7,0.7,0.85,0.85);
     leg2->AddEntry(gDT3,"method 1","P");
     leg2->AddEntry(gDT4,"method 2","P");
     leg2->Draw();

     TCanvas *c4 = new TCanvas("c4","c4",1000,1000);
     gRate_ch0_1->SetMarkerStyle(8);
     gRate_ch0_1->SetMarkerColor(2);

     TMultiGraph *mg3 = new TMultiGraph();
     mg3->Add(gRate_ch0_1);
     //mg3->Add(gRate_ch0_n);
     mg3->Draw("AP");
     mg3->SetTitle(";bufferlevel;rate;");

     TLegend *leg3 = new TLegend(0.7,0.7,0.85,0.85);
     leg3->AddEntry(gRate_ch0_1,"measured rate","P");
     //leg3->AddEntry(gRate_ch0_n,"expected rate","P");
     leg3->Draw();

}

