#include "CalcDT.C"

void PlotAll(){
     const int np=2; 
     int run_number[np]={306,307};
     int nn[np] = {3,2};

     TGraph *gDT1 = new TGraph(8);
     TGraph *gDT2 = new TGraph(8);

     for(int ii=0; ii<np; ii++){
	Double_t dt1=0, dt2=0;
	CalcDT(run_number[ii],dt1,dt2);

	gDT1->SetPoint(ii,nn[ii],1-dt1);
	gDT2->SetPoint(ii,nn[ii],1-dt2);
        cout<<dt1<<"  "<<dt2<<endl;
     }

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
}

