void PrintHel(){
    int nrun=0;
    cout<<"Which run?  ";
    cin>>nrun;

    TString filename = Form("/home/daq/work/SOLID_DAQ/decoder/Rootfiles/fadctest_%d.root", nrun);

    Int_t nevent=0;
    cout<<"How many helicitties? (173+n): n= ";
    cin>>nevent;


    TFile *f0 = new TFile(filename);
    TTree *VTP = (TTree*) f0->Get("VTP");

    Int_t past_hel[6];
    Int_t hel_win_cnt=0;

    VTP->SetBranchAddress("vtp_past_hel",past_hel);
    VTP->SetBranchAddress("hel_win_cnt",&hel_win_cnt);

    Int_t nentries=VTP->GetEntries();
    
    Int_t pre_hel_win=0;
    Int_t nhel=0;

    int nn=0;
    for(int ii=0; ii<nentries; ii++){
	VTP->GetEntry(ii);

	if(ii==0) pre_hel_win=hel_win_cnt;

	Int_t cur_hel=hel_win_cnt;
	if(ii==0){
	   for(int jj=0; jj<6; jj++){
	     int nbit=30;
	     if(jj==5) nbit=23;
	     unsigned int bitmask= (0x1 <<(nbit-1));
	     for(int kk=0; kk<nbit; kk++){
		int bit= (past_hel[5-jj] & bitmask);
		bit = bit>>(nbit-1-kk);
		cout<<bit;
		//cout<<nn<<"  "<<bit<<endl;
		nn++;
		bitmask=bitmask>>1;
	     }
	   }    
	   nhel++;
	}

	if(cur_hel!=pre_hel_win){
	   int nwin_diff = cur_hel-pre_hel_win;
	   if(nwin_diff>1) cout<<"Something wrong with the helicity window counts"<<endl;

	   cout<<(past_hel[0] & 0x1);
	   nhel++;
	}	

	if(nhel>nevent)break;
    } 
    cout<<endl;
    cout<<nn<<endl;
}
