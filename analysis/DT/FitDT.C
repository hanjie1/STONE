void FitDT(){
    ifstream infile;
    infile.open("deadtime_rawmode.txt");
	
    Ssiz_t from=0;
    TString content,tmp;
    int nn=0;

    Double_t rate1[8]={0},dt1[8]={0},dt1_err[8]={0}; // only two channels enabled

    while(tmp.ReadLine(infile)){
        from=0;
	if(nn>=8)continue;
          tmp.Tokenize(content,from,"  ");
          rate1[nn]=atof(content.Data());
          tmp.Tokenize(content,from,"  ");
          dt1[nn]=atof(content.Data());
          tmp.Tokenize(content,from," ");
          dt1_err[nn]=atof(content.Data());
        nn++;
    }
    infile.close();

    TGraphErrors *gDT1 = new TGraphErrors();

    for(int ii=0; ii<8; ii++){
	gDT1->SetPoint(ii,rate1[ii],1-dt1[ii]);
	gDT1->SetPointError(ii,0,dt1_err[ii]);
    }


    gDT1->SetMarkerStyle(8);
    gDT1->SetMarkerColor(4);

    gDT1->Fit("pol6");
    gDT1->GetFunction("pol6")->SetLineColor(4);

    
    gDT1->Draw("AP");
    gDT1->SetTitle(";rate;dead time;");

    TLegend *leg = new TLegend(0.7,0.7,0.85,0.85);
    leg->AddEntry(gDT1,"raw mode","P");
    leg->Draw();



}
