void FitDT2(){
    ifstream infile1;
    infile1.open("deadtime_rawmode_vme.txt");
	
    Ssiz_t from=0;
    TString content,tmp;
    int nn=0;

    Double_t rate1[7]={0},dt1[7]={0},dt1_err[7]={0}; 

    while(tmp.ReadLine(infile1)){
        from=0;
        tmp.Tokenize(content,from,"  ");
        rate1[nn]=atof(content.Data());
        tmp.Tokenize(content,from,"  ");
        dt1[nn]=atof(content.Data());
        tmp.Tokenize(content,from," ");
        dt1_err[nn]=atof(content.Data());
        nn++;
    }
    infile1.close();

    ifstream infile2;
    infile2.open("deadtime_rawmode_fastreadout.txt");
	
    nn=0;
    Double_t rate2[7]={0},dt2[7]={0},dt2_err[7]={0}; 

    while(tmp.ReadLine(infile2)){
        from=0;
        tmp.Tokenize(content,from,"  ");
        rate2[nn]=atof(content.Data());
        tmp.Tokenize(content,from,"  ");
        dt2[nn]=atof(content.Data());
        tmp.Tokenize(content,from," ");
        dt2_err[nn]=atof(content.Data());
        nn++;
    }
    infile2.close();

    TGraphErrors *gDT1 = new TGraphErrors();
    TGraphErrors *gDT2 = new TGraphErrors();

    for(int ii=0; ii<7; ii++){
	gDT1->SetPoint(ii,rate1[ii],1-dt1[ii]);
	gDT1->SetPointError(ii,0,dt1_err[ii]);

	gDT2->SetPoint(ii,rate2[ii],1-dt2[ii]);
	gDT2->SetPointError(ii,0,dt2_err[ii]);
    }

    gStyle->SetOptFit(1111);

    TCanvas *c1 = new TCanvas("c1","c1",1000,500);

    TMultiGraph *mg = new TMultiGraph();
    gDT1->SetMarkerStyle(8);
    gDT1->SetMarkerColor(4);
    gDT1->SetMarkerSize(1.2);
    gDT2->SetMarkerStyle(22);
    gDT2->SetMarkerColor(2);
    gDT2->SetMarkerSize(1.2);

    gDT1->Fit("pol2","");
    gDT1->GetFunction("pol2")->SetLineColor(4);

    gDT2->Fit("pol2","");
    gDT2->GetFunction("pol2")->SetLineColor(2);

    mg->Add(gDT1);
    mg->Add(gDT2);
    mg->Draw("AP");
    mg->SetTitle(";rate(Hz);dead time;");

    TLegend *leg = new TLegend(0.2,0.7,0.35,0.85);
    leg->AddEntry(gDT1,"VME readout","P");
    leg->AddEntry(gDT2,"VTP readout","P");
    leg->Draw();

    TLatex tex;;
   tex.SetTextSize(0.03);
   //tex.DrawLatexNDC(.5,.3,Form("y = %.3e + %.3e x",p0,p1));
   tex.DrawLatexNDC(.2,.6,"PTW = 128 ns");

//    c1->Print("DT_rawmode_1kHz.pdf");

}
