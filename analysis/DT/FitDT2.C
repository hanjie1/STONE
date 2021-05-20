void FitDT2(){
    ifstream infile;
    infile.open("deadtime_rawmode.txt");
	
    Ssiz_t from=0;
    TString content,tmp;
    int nn=0;

    Double_t rate1[6]={0},dt1[6]={0},dt1_err[6]={0}; 
    Double_t rate2[13]={0},dt2[13]={0},dt2_err[13]={0}; 

    while(tmp.ReadLine(infile)){
        from=0;
        if(nn<6){
          tmp.Tokenize(content,from,"  ");
          rate1[nn]=atof(content.Data());
          tmp.Tokenize(content,from,"  ");
          dt1[nn]=atof(content.Data());
          tmp.Tokenize(content,from," ");
          dt1_err[nn]=atof(content.Data());
        }
	else{
          tmp.Tokenize(content,from,"  ");
          rate2[nn-6]=atof(content.Data());
          tmp.Tokenize(content,from,"  ");
          dt2[nn-6]=atof(content.Data());
          tmp.Tokenize(content,from," ");
          dt2_err[nn-6]=atof(content.Data());
        }
        nn++;
    }
    infile.close();

    TGraphErrors *gDT1 = new TGraphErrors();
    TGraphErrors *gDT2 = new TGraphErrors();

    for(int ii=0; ii<6; ii++){
     if(rate1[ii]<20000){
	gDT1->SetPoint(ii,rate1[ii],1-dt1[ii]);
	gDT1->SetPointError(ii,0,dt1_err[ii]);
      }
    }

    for(int ii=0; ii<13; ii++){
      if(rate2[ii]<16000){
	gDT2->SetPoint(ii,rate2[ii],1-dt2[ii]);
	gDT2->SetPointError(ii,0,dt2_err[ii]);
      }
    }

    gStyle->SetOptFit(1111);

    TCanvas *c1 = new TCanvas("c1","c1",1000,500);

    TMultiGraph *mg = new TMultiGraph();
    gDT1->SetMarkerStyle(8);
    gDT1->SetMarkerColor(4);
    gDT2->SetMarkerStyle(22);
    gDT2->SetMarkerColor(4);

//    gDT1->Fit("pol4");
//    gDT1->GetFunction("pol4")->SetLineColor(4);

//    gDT2->Fit("pol5");
    //gDT2->GetFunction("pol6")->SetLineColor(4);

    TF1 *f2 = new TF1("f2","pol1",0,18000);
    gDT2->Fit(f2,"","",0,18000);
   
    double p0=f2->GetParameter(0);
    double p0_err=f2->GetParameter(0);
    double p1=f2->GetParameter(1);
  
//    mg->Add(gDT1);
    mg->Add(gDT2);
    mg->Draw("AP");
    //f2->Draw("same");
    mg->SetTitle(";rate(Hz);dead time;");
    gPad->Modified();
    mg->GetXaxis()->SetLimits(0,16000);
    gPad->Modified();
    mg->GetYaxis()->SetRangeUser(0,0.005);

    TLegend *leg = new TLegend(0.2,0.7,0.35,0.85);
 //   leg->AddEntry(gDT1,"wide pulse (PTW=192ns)","P");
    leg->AddEntry(gDT2,"narrow pulse (PTW=128ns)","P");
   // leg->Draw();

    TLatex tex;;
   tex.SetTextSize(0.03);
   //tex.DrawLatexNDC(.5,.3,Form("y = %.3e + %.3e x",p0,p1));
   tex.DrawLatexNDC(.7,.2,"PTW = 128 ns");

    c1->Print("DT_rawmode_1kHz.pdf");

}
