void FitDT2(){
    ifstream infile;
    infile.open("deadtime1.txt");
	
    Ssiz_t from=0;
    TString content,tmp;
    int nn=0;

    Double_t rate1[8]={0},dt1[8]={0},dt1_err[8]={0}; 
    Double_t rate2[8]={0},dt2[8]={0},dt2_err[8]={0}; 

    while(tmp.ReadLine(infile)){
        from=0;
        if(nn<8){
          tmp.Tokenize(content,from,"  ");
          rate1[nn]=atof(content.Data());
          tmp.Tokenize(content,from,"  ");
          dt1[nn]=atof(content.Data());
          tmp.Tokenize(content,from," ");
          dt1_err[nn]=atof(content.Data());
        }
	else{
          tmp.Tokenize(content,from,"  ");
          rate2[nn-8]=atof(content.Data());
          tmp.Tokenize(content,from,"  ");
          dt2[nn-8]=atof(content.Data());
          tmp.Tokenize(content,from," ");
          dt2_err[nn-8]=atof(content.Data());
        }
        nn++;
    }
    infile.close();

    TGraphErrors *gDT1 = new TGraphErrors();
    TGraphErrors *gDT2 = new TGraphErrors();

    for(int ii=0; ii<8; ii++){
	gDT1->SetPoint(ii,rate1[ii],1-dt1[ii]);
	gDT1->SetPointError(ii,0,dt1_err[ii]);
    }

    for(int ii=0; ii<8; ii++){
	gDT2->SetPoint(ii,rate2[ii],1-dt2[ii]);
	gDT2->SetPointError(ii,0,dt2_err[ii]);
    }



    TMultiGraph *mg = new TMultiGraph();
    gDT1->SetMarkerStyle(8);
    gDT1->SetMarkerColor(4);
    gDT2->SetMarkerStyle(22);
    gDT2->SetMarkerColor(2);

    gDT1->Fit("pol4");
    gDT1->GetFunction("pol4")->SetLineColor(4);

    gDT2->Fit("pol5");
    //gDT2->GetFunction("pol6")->SetLineColor(4);
/*
    TF1 *f2 = new TF1("f2","pol3",1200,180000);
    gDT2->Fit(f2,"","",1200,60000);
*/  
    mg->Add(gDT1);
    mg->Add(gDT2);
    mg->Draw("AP");
    //f2->Draw("same");
    mg->SetTitle(";rate(Hz);dead time;");

    TLegend *leg = new TLegend(0.7,0.7,0.85,0.85);
    leg->AddEntry(gDT1,"wide pulse","P");
    leg->AddEntry(gDT2,"narrow pulse","P");
    leg->Draw();



}
