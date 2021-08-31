void Countdiff(){
     int nrun=0;
    cout<<"Which run?  ";
    cin>>nrun;

    TString filename = Form("/home/daq/work/SOLID_DAQ/decoder/Rootfiles/fadctest_%d.root", nrun);

    TFile *f0 = new TFile(filename);
    TTree *T = (TTree*) f0->Get("T");
    TTree *VTP = (TTree*) f0->Get("VTP");
    T->AddFriend(VTP);


    Int_t fadc_scal[16];
    int clust_x, clust_y, clust_e, clust_t, clust_n;    


    T->SetBranchAddress("fadc_scal_cnt",fadc_scal);
    T->SetBranchAddress("clust_y",&clust_y);
    T->SetBranchAddress("clust_e",&clust_e);
    T->SetBranchAddress("clust_x",&clust_x);
    T->SetBranchAddress("clust_n",&clust_n);
    T->SetBranchAddress("clust_t",&clust_t);

    Int_t nentries = T->GetEntries();

    Int_t scal_c[5]={0};
    Int_t fadc_c[5]={0};

    Int_t tmpcnt[5]={0};

    for(int ii=0; ii<nentries; ii++){
	T->GetEntry(ii);
	
	if(clust_y==1) tmpcnt[0]++;
	if(clust_y==5) tmpcnt[1]++;
	if(clust_y==2) tmpcnt[2]++;
	if(clust_y==4) tmpcnt[3]++;
	if(clust_y==3) tmpcnt[4]++;

	if(fadc_scal[5]>0){
	   scal_c[0] = fadc_scal[5];
	   fadc_c[0] = fadc_c[0]+tmpcnt[0];
	   tmpcnt[0]=0;
	}	

	if(fadc_scal[15]>0){
	   scal_c[1] = fadc_scal[15];
	   fadc_c[1] = fadc_c[1]+tmpcnt[1];
	   tmpcnt[1]=0;
	}	

	if(fadc_scal[11]>0){
	   scal_c[2] = fadc_scal[11];
	   fadc_c[2] = fadc_c[2]+tmpcnt[2];
	   tmpcnt[2]=0;
	}	

	if(fadc_scal[3]>0){
	   scal_c[3] = fadc_scal[3];
	   fadc_c[3] = fadc_c[3]+tmpcnt[3];
	   tmpcnt[3]=0;
	}	

	if(fadc_scal[7]>0){
	   scal_c[4] = fadc_scal[7];
	   fadc_c[4] = fadc_c[4]+tmpcnt[4];
	   tmpcnt[4]=0;
	}	


    }

    double dt[5]={0};
    for(int ii=0; ii<5; ii++){
	dt[ii] = (1.0*fadc_c[ii])/scal_c[ii];
	cout<<dt[ii]<<endl;
    }

}
