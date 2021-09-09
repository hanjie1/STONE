void Countdiff(){
    int nrun=0;
    cout<<"Which run?  ";
    cin>>nrun;

   auto fileName = Form("/home/daq/work/SOLID_DAQ/decoder/Rootfiles/fadctest_%d.root",nrun);
   auto treeName = "T";
   ROOT::RDataFrame d(treeName, fileName);

   auto scalmax_5 = d.Filter("fadc_scal_cnt[5]>0").Define("scal5","fadc_scal_cnt[5]").Max("scal5");         // cluster 0
   auto scalmax_9 = d.Filter("fadc_scal_cnt[9]>0").Define("scal9","fadc_scal_cnt[9]").Max("scal9");         // cluster 1
   auto scalmax_11 = d.Filter("fadc_scal_cnt[11]>0").Define("scal11","fadc_scal_cnt[11]").Max("scal11");    // cluster 2
   auto scalmax_3 = d.Filter("fadc_scal_cnt[3]>0").Define("scal3","fadc_scal_cnt[3]").Max("scal3");         // cluster 3
   auto scalmax_7 = d.Filter("fadc_scal_cnt[7]>0").Define("scal7","fadc_scal_cnt[7]").Max("scal7");	    // cluster 4

   auto treeName1 = "VTP";
   ROOT::RDataFrame d_vtp(treeName1, fileName);

   auto fadc_c1 = d_vtp.Filter("clust_y==1").Count();  // cluster 0
   auto fadc_c2 = d_vtp.Filter("clust_y==5").Count();  // cluster 1
   auto fadc_c3 = d_vtp.Filter("clust_y==2").Count();  // cluster 2
   auto fadc_c4 = d_vtp.Filter("clust_y==4").Count();  // cluster 3
   auto fadc_c5 = d_vtp.Filter("clust_y==3").Count();  // cluster 4

   double dt1=(*fadc_c1)/(*scalmax_5);
   double dt2=(*fadc_c2)/(*scalmax_9);
   double dt3=(*fadc_c3)/(*scalmax_11);
   double dt4=(*fadc_c4)/(*scalmax_3);
   double dt5=(*fadc_c5)/(*scalmax_7);

   cout<<dt1<<endl;
   cout<<dt2<<endl;
   cout<<dt3<<endl;
   cout<<dt4<<endl;
   cout<<dt5<<endl;
   
}
