const int NCLUST=10;

typedef struct{
   int nx;
   int ny;
}block_coords;

block_coords block_map[16]={
       {1,1},{1,2},{1,3},{1,4},{1,5},{2,1},{2,2},{2,3},{2,4},{2,5},{3,1},{3,2},{3,3},{3,4},{3,5},{3,6}
};

int Find_block(int ch,int dim){    
  int nx=0; 
  int ny=0;

  nx=block_map[ch].nx;
  ny=block_map[ch].ny;

  if( dim==0 ) return nx;
  else return ny;

}

// for a given (x,y), return the channel number
int Find_channel(int nx, int ny){
  int ch = -1;

  int found=0;
  for(int jj=0;jj<16;jj++){
      if( (block_map[jj].nx==nx) && (block_map[jj].ny==ny) ){
	   ch = jj;
      }
   }

  return ch;
}
 
int Find_nearby(int ch, int ii){
     int nx=0, ny=0;
     nx = Find_block(ch,0);
     ny = Find_block(ch,1);

     switch(ii){
       case 0: return Find_channel(nx, ny);     // center
       case 1: return Find_channel(nx-1, ny-1); // left up
       case 2: return Find_channel(nx-1, ny);   // left down
       case 3: return Find_channel(nx, ny-1);   // middle up
       case 4: return Find_channel(nx, ny+1);   // middle down
       case 5: return Find_channel(nx+1, ny);   // right up
       case 6: return Find_channel(nx+1, ny+1); // right down
     }

     return -1;
}

void FindCluster(Int_t rawADC[16][32], int *nhit, int *x, int *y){
    
    int NPED=200;
    int NSA=14;

    int e_sum=0;
    int t[16]={0};
    int e[16]={0};

    for(int ii=0; ii<16; ii++){

     int tmp_esum=0;
     int count=0;

     bool start=false;
     for(int jj=0; jj<32; jj++){
	if(rawADC[ii][jj]>=NPED && count==0){
	   t[ii]=jj;
	   tmp_esum += rawADC[ii][jj-1];
	   tmp_esum += rawADC[ii][jj-2];
	   start=true;
	}	
	if(count<NSA && start){
	   count++;
	   tmp_esum += rawADC[ii][jj];
	}  
     }

     e[ii]=tmp_esum;
   }

   int nclust=0;
   int clust_e[NCLUST]={0};
   int maxe=0;
   int ch=0;
   int nhitc[NCLUST]={1,1,1,1,1,1,1,1,1,1};

   bool bad=false;

   for(int ii=0; ii<16; ii++){
     bad=false;

     if(e[ii]<5000) continue;
     for(int jj=1; jj<=6; jj++){
         int tmpch=Find_nearby(ii, jj);
         if(tmpch==-1) continue;
	 if(e[tmpch]>e[ii]) { bad=true; nhitc[nclust]=1; break;}
	 if(e[tmpch]==0) continue;
	 
	 if(abs(t[tmpch]-t[ii])<=4){ nhitc[nclust]++;	clust_e[nclust]+=e[tmpch];}
      }
      if(nhitc[nclust]>0 && bad==false){
         clust_e[nclust]+=e[ii];
	  //if(clust_e[nclust]<10000) continue;

	  x[nclust] = Find_block(ii,0);   
	  y[nclust] = Find_block(ii,1);   
	  nhit[nclust] = nhitc[nclust];

	  nclust++;
	}
   } 

   return;
}

void SearchAndCheck(){

     int nrun=0;
    cout<<"Which run?  ";
    cin>>nrun;

    TString filename = Form("/home/daq/work/SOLID_DAQ/decoder/Rootfiles/fadctest_%d.root", nrun);

    TFile *f0 = new TFile(filename);
    TTree *T = (TTree*) f0->Get("T");
    TTree *VTP = (TTree*) f0->Get("VTP");
    T->AddFriend(VTP);

    Int_t fadc_scal[16];
    Int_t rawADC[16][32];
    int clust_x[NCLUST], clust_y[NCLUST], clust_e[NCLUST], clust_t[NCLUST], clust_n[NCLUST];

    T->SetBranchAddress("fadc_rawADC",rawADC);
    T->SetBranchAddress("clust_y",clust_y);
    T->SetBranchAddress("clust_e",clust_e);
    T->SetBranchAddress("clust_x",clust_x);
    T->SetBranchAddress("clust_n",clust_n);
    T->SetBranchAddress("clust_t",clust_t);

    Int_t nentries = T->GetEntries();

    TH2F *hxy = new TH2F("hxy","hxy",10,0,10,10,0,10);
    TH2F *hny = new TH2F("hny","hny",10,0,10,10,0,10);

    TH2F *hxy_r = new TH2F("hxy_r","hxy_r",10,0,10,10,0,10);
    TH2F *hny_r = new TH2F("hny_r","hny_r",10,0,10,10,0,10);

    for(int ii=0; ii<nentries; ii++){
    //for(int ii=0; ii<2071; ii++){
        T->GetEntry(ii);

 	int nhit[NCLUST]={0}, xx[NCLUST]={0}, yy[NCLUST]={0};
	FindCluster(rawADC, &nhit[0], &xx[0], &yy[0]);

	for(int jj=0; jj<NCLUST; jj++){
	  if(yy[jj]>0 && xx[jj]>0) hxy_r->Fill(yy[jj], xx[jj]);	
	  if(clust_y[jj]>0 && clust_x[jj]>0) hxy->Fill(clust_y[jj], clust_x[jj]);	

	  if(nhit[jj]>0) hny_r->Fill(nhit[jj], yy[jj]);
	  if(clust_n[jj]>0) hny->Fill(clust_n[jj], clust_y[jj]);

	}
    }

    TCanvas *c1=new TCanvas("c1","c1",1000,1000);
    c1->Divide(2,2);
    c1->cd(1);
    hxy->Draw("COLZ");
    hxy->SetTitle("VTP cluster;clust.y;clust.x;");
    c1->cd(2);
    hxy_r->Draw("COLZ");
    hxy_r->SetTitle("FADC cluster;clust.y;clust.x;");
    c1->cd(3);
    hny->Draw("COLZ");
    hny->SetTitle("VTP cluster;clust.n;clust.y;");
    c1->cd(4);
    hny_r->Draw("COLZ");
    hny_r->SetTitle("FADC cluster;clust.n;clust.y;");
}
