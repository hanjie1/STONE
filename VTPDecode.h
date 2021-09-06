struct vtp_data_struct
{
   unsigned int type;
   unsigned int new_type;
   unsigned int sub_type;
   unsigned int slot_id_hd;
   unsigned int slot_id_tr;
   unsigned int blk_num;
   unsigned int blk_size;
   unsigned int num_words;
   unsigned int dev_id;
   unsigned int trig_num;
   unsigned int trig_time_l;
   unsigned int trig_time_h;
   unsigned int last_mps_t;
   unsigned int hel_win_cnt;   // helicity window counts in helicity
   unsigned int helicity[6];
   unsigned int hel_win_cnt_1;  // helicity window counts in helicity based trigger scalers
   unsigned int livetime;
   unsigned int busytime;
   unsigned int trig_cnt[5];   // trigger counts for each TS bit
   unsigned int slot_id_strip; // slot id for scaler counts
   unsigned int chan;          // channel of a slot for scaler counts
   unsigned int scal_cnt;      // scaler counts
   unsigned int trig_pat_time;     
   unsigned int trig_pat;
   unsigned int clust_y[NCLUST];       // cluster y
   unsigned int clust_x[NCLUST];       // cluster x
   unsigned int clust_n[NCLUST];       // cluster n
   unsigned int clust_t[NCLUST];       // cluster t
   unsigned int clust_e[NCLUST];       // cluster e
}vtp_data;

int sub_type_8 = 0;
int sub_type_9 = 0;
int sub_type_10 = 0;
int nclust=0;

void ClearVTP(){
   for(int ii=0;ii<6;ii++){
	vtp_data.helicity[ii] = 0;
   }

   for(int ii=0;ii<NCLUST;ii++){
	vtp_data.clust_x[ii] = 0;
	vtp_data.clust_y[ii] = 0;
	vtp_data.clust_e[ii] = 0;
	vtp_data.clust_t[ii] = 0;
	vtp_data.clust_n[ii] = 0;
   }
    sub_type_8 = 0;
    sub_type_9 = 0;
    sub_type_10 = 0;
    nclust=0;
}

void vtpDataDecode(unsigned int data){

  int i_print = 0;
  static unsigned int type_last = 15;   /* initialize to type FILLER WORD */
  static unsigned int sub_type_last = 0;   /* initialize to type FILLER WORD */
  static unsigned int iword=0;

// Note,  vtp_data_struct vtp_data is global, see top of code

  if (i_print) printf("%3d: ",iword++);

  if( data & 0x80000000 )       /* data type defining word */
    {   
      vtp_data.new_type = 1;
      vtp_data.type = (data & 0x78000000) >> 27; 
    }   
  else
    {   
      vtp_data.new_type = 0;
      vtp_data.type = type_last;
	  vtp_data.sub_type = sub_type_last;
    }   

//  if(i_print)
//	printf(" TYPE = %d, SUB TYPE = %d\n",vtp_data.type,vtp_data.sub_type);

  switch( vtp_data.type )
  {
    case 0:     /* BLOCK HEADER */
        vtp_data.slot_id_hd = ((data) & 0x7C00000) >> 22;
        vtp_data.blk_num = (data >> 8 ) & 0x3FF;
        vtp_data.blk_size = (data & 0xFF);
        if( i_print )
            printf("%8X - BLOCK HEADER - slot = %d   n_evts = %d   n_blk = %d\n",
           data, vtp_data.slot_id_hd, vtp_data.blk_size, vtp_data.blk_num);
      break;
    case 1:     /* BLOCK TRAILER */
      vtp_data.slot_id_tr = (data & 0x7C00000) >> 22;
      vtp_data.num_words = (data & 0x3FFFFF);
      if( i_print )
        printf("%8X - BLOCK TRAILER - slot = %d   num_words = %d\n",
           data, vtp_data.slot_id_tr, vtp_data.num_words);
      break;
    case 2:    /* Event Header */
	  vtp_data.dev_id = (data & 0x7efffff) >> 21;
	  vtp_data.trig_num = (data & 0x1fffff);
      if( i_print )
		 printf("%8X - EVENT HEADER - dev_id = %d, trigger number = %d\n",
		      data, vtp_data.dev_id, vtp_data.trig_num);
	  break;
	case 3:    /* trigger time */
	  if(vtp_data.new_type){
		 vtp_data.trig_time_l = (data & 0xffffff);
	     if(i_print)
		    printf("%8X - TRIGGER TIME - trigger time lower = %d\n", data, vtp_data.trig_time_l);
	  }
	  else{
		 vtp_data.trig_time_h = (data & 0xffffff);
	     if(i_print)
		    printf("%8X - TRIGGER TIME - trigger time upper = %d\n", data, vtp_data.trig_time_h);
		 vtp_trigtime = ( vtp_data.trig_time_h<<24 ) | vtp_data.trig_time_l;
	  }
	  break;
    case 12:
	  if(vtp_data.new_type)
	    vtp_data.sub_type = (data & 0x78fffff) >> 23;

	  switch(vtp_data.sub_type){
		case 8:
		  if(vtp_data.new_type){
			 sub_type_8=0;
		     vtp_data.helicity[sub_type_8] = (data & 0x7fffff);
			 if(i_print)
				printf("%8X - HELICITY - PAST HELICITY 0 = %d\n", data, vtp_data.helicity[0]);
	      }
		  else{
			 if(sub_type_8 == 1){
				vtp_data.last_mps_t = (data & 0x7fffffff);
				last_mps_time = vtp_data.last_mps_t;
			    if(i_print)
				  printf("%8X - HELICITY - LAST MPS TIME = %d\n", data, vtp_data.last_mps_t);
			 }	
			 if(sub_type_8 == 2){
				vtp_data.hel_win_cnt = (data & 0x7fffffff);
				hel_win_cnt = vtp_data.hel_win_cnt;
			    if(i_print)
				  printf("%8X - HELICITY - HELICITY WINDOW COUNTS = %d\n", data, vtp_data.hel_win_cnt);
			 }	
			 if(sub_type_8 > 2 && sub_type_8 < 8){
				vtp_data.helicity[sub_type_8-2] = (data & 0x7fffffff);
			    if(i_print)
				  printf("%8X - HELICITY - PAST HELICITY %d = %d\n", data, sub_type_8-2, vtp_data.helicity[sub_type_8-2]);
			 }	
		  }
		  sub_type_8++;
		  if(sub_type_8 > 8){
			printf("VTP Warning: number of words in type 12.8 %d > 8\n",sub_type_8);
		  }
		  break;
		case 9:
		  if(vtp_data.new_type){
			 sub_type_9 = 0;
			 if(i_print)
				printf("%8X - TRIGGER SCALER - \n",data);
	      }
		  else{
			 if(sub_type_9 == 1){
				vtp_data.hel_win_cnt_1 = (data & 0x7fffffff);
				hel_win_cnt_1 = vtp_data.hel_win_cnt_1;
			    if(i_print)
				  printf("%8X - TRIGGER SCALER - HELICITY WINDOW COUNTS = %d\n", data, vtp_data.hel_win_cnt_1);
			 }	
			 if(sub_type_9 == 2){
				vtp_data.livetime = (data & 0x7fffffff);
				livetime = vtp_data.livetime;
			    if(i_print)
				  printf("%8X - TRIGGER SCALER - LIVE TIME = %d\n", data, vtp_data.livetime);
			 }	
			 if(sub_type_9 == 3){
				vtp_data.busytime = (data & 0x7fffffff);
				busytime = vtp_data.busytime;
			    if(i_print)
				  printf("%8X - TRIGGER SCALER - BUSY TIME = %d\n", data, vtp_data.busytime);
			 }	
			 if(sub_type_9 > 3 && sub_type_9 < 9){
				vtp_data.trig_cnt[sub_type_9-4] = (data & 0x7fffffff);
				trigcnt[sub_type_9-4] = vtp_data.trig_cnt[sub_type_9-4];
			    if(i_print)
				  printf("%8X - TRIGGER SCALER -  %d = %d\n", data, sub_type_9-2, vtp_data.last_mps_t);
			 }	
		  }
		  sub_type_9++;
		  if(sub_type_9 > 9){
			printf("VTP Warning: number of words in type 12.9 %d > 9\n",sub_type_9);
		  }
	     break;
	   case 10:
		  if(vtp_data.new_type){
			 sub_type_10 = 0;
			 vtp_data.slot_id_strip = (data & 0x1f);
			 if(i_print)
				printf("%8X - VTP SCALER - HEADER - SLOT ID = %d\n",data,vtp_data.slot_id_strip);
	          }
		  else{
			 if(sub_type_10 < 129){
				vtp_data.chan = (data >> 24) & 0x7f ;
				vtp_data.scal_cnt = (data & 0xffffff);

			    if(i_print)
				  printf("%8X - VTP SCALER - CHAN %d scaler counts = %d\n", data, vtp_data.chan, vtp_data.scal_cnt);

                            if(vtp_data.slot_id_strip == FADC_SLOT)   vtp_fadc_scalcnt[vtp_data.chan]=vtp_data.scal_cnt;
			 }	
		  }
		  sub_type_10++;
		  if(sub_type_10 > 129){
			printf("VTP Warning: number of words in type 12.10 %d > 129\n",sub_type_10);
		  }
	     break;
	   case 11:
		  if(vtp_data.new_type){
			 vtp_data.clust_e[nclust] = (data & 0xffff);
			 if(i_print)
				printf("%8X - VTP CLUSTER - HEADER\n",data);
	          }
		  else{
			if(nclust<NCLUST){
			 vtp_data.clust_y[nclust]=(data >> 19) & 0xf;			 
			 vtp_data.clust_x[nclust]=(data >> 14) & 0x1f;			 
			 vtp_data.clust_n[nclust]=(data >> 11) & 0x7;			 
			 vtp_data.clust_t[nclust]=(data & 0x7ff);			 

			 if(i_print)
			    printf("%8X - VTP CLUST - (x, y, n, t, e) = (%d, %d, %d, %d, %d)\n", data, vtp_data.clust_x[nclust], vtp_data.clust_y[nclust], 
				    vtp_data.clust_n[nclust], vtp_data.clust_t[nclust],vtp_data.clust_e[nclust]);
			}
			else printf("VTPDecoder warning: found %d cluster > %d\n",nclust+1,NCLUST);

			nclust++;
		  }
	     break;

	  } 
	break;

	case 13:
	 vtp_data.trig_pat_time = (data >> 16) & 0x7ff;
	 vtp_data.trig_pat = (data & 0xffff);
	 trig_pattern[pattern_num] = vtp_data.trig_pat;
	 trig_pattern_time[pattern_num] = vtp_data.trig_pat_time;
	 pattern_num++;
	 if(i_print)
		printf("%8X - TRIGGER BIT - TIME = %d, BIT PATTERN = %d\n",data,vtp_data.trig_pat_time, vtp_data.trig_pat);
	 break;

	case 14:
	 if(i_print)
		printf("%8X - DATA NOT VALID\n",data);
	 break;

	case 15:
	 if(i_print)
		printf("%8X - FILLER WORD\n",data);
	 break;
  }

  type_last = vtp_data.type;
  sub_type_last = vtp_data.sub_type;

}
