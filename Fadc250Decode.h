struct fadc_data_struct
{
  unsigned int new_type;
  unsigned int type;
  unsigned int slot_id_hd;
  unsigned int slot_id_tr;
  unsigned int slot_id_evt;
  unsigned int n_evts;
  unsigned int blk_num;
  unsigned int n_words;
  unsigned int evt_num_1;
  unsigned int evt_num_2;
  unsigned int time_now;
  unsigned int time_1;
  unsigned int time_2;
  unsigned int chan;
  unsigned int width;
  unsigned int valid_1;
  unsigned int adc_1;
  unsigned int valid_2;
  unsigned int adc_2;
  unsigned int over;
  unsigned int adc_sum;
  unsigned int pulse_num;
  unsigned int thres_bin;
  unsigned int quality;
  unsigned int integral;
  unsigned int time;
  unsigned int chan_a;
  unsigned int source_a;
  unsigned int chan_b;
  unsigned int source_b;
  unsigned int group;
  unsigned int time_coarse;
  unsigned int time_fine;
  unsigned int vmin;
  unsigned int vpeak;
  unsigned int trig_type_int;   /* next 4 for internal trigger data */
  unsigned int trig_state_int;  /* e.g. helicity */
  unsigned int evt_num_int;
  unsigned int err_status_int;
}fadc_data;

#define RAW_MODE 1
#define PULSE_RAW_MODE 2
#define PULSE_INT_MODE 3
#define MAXHIT  10

int data_type_4=0;
int data_type_6=0;
int data_type_7=0;
int data_type_8=0;

Int_t nsamples = 0;
Int_t trignum = 0;
Int_t nrawdata = 0;
unsigned int mychan = 0;
int ftdc_nhit[16] = {0};
unsigned int oldchan=-1;

int GetFadcMode(){
    int mode = -1;    
	if(data_type_4  && !(data_type_6) && !(data_type_7) && !(data_type_8)) mode = 1;
	else if(!(data_type_4) && data_type_6    && !(data_type_7) && !(data_type_8)) mode = 2;
	else if(!(data_type_4) && !(data_type_6) && data_type_7 && data_type_8 ) mode = 3;
    return mode;
}

void faDataDecode(unsigned int data)
{ 

  int i_print = 0;
  static unsigned int type_last = 15;   /* initialize to type FILLER WORD */
  static unsigned int time_last = 0;
  static unsigned int iword=0;


// Note,  fadc_data_struct fadc_data is global, see top of code

  if (i_print) printf("%3d: ",iword++);

  if( data & 0x80000000 )       /* data type defining word */
    {
      fadc_data.new_type = 1;
      fadc_data.type = (data & 0x78000000) >> 27;
    }
  else
    {
      fadc_data.new_type = 0;
      fadc_data.type = type_last;
    }

  switch( fadc_data.type )
    {
    case 0:     /* BLOCK HEADER */
	  if(fadc_data.new_type){
        fadc_data.slot_id_hd = ((data) & 0x7C00000) >> 22;
        fadc_data.blk_num = (data >> 8 ) & 0x3FF;
        fadc_data.n_evts = (data & 0xFF);
        if( i_print )
            printf("%8X - BLOCK HEADER - slot = %d   n_evts = %d   n_blk = %d\n",
           data, fadc_data.slot_id_hd, fadc_data.n_evts, fadc_data.blk_num);
	  }
      break;
    case 1:     /* BLOCK TRAILER */
      fadc_data.slot_id_tr = (data & 0x7C00000) >> 22;
      fadc_data.n_words = (data & 0x3FFFFF);
      if( i_print )
    printf("%8X - BLOCK TRAILER - slot = %d   n_words = %d\n",
           data, fadc_data.slot_id_tr, fadc_data.n_words);
      break;
    case 2:     /* EVENT HEADER */
      if( fadc_data.new_type )
    {
	  fadc_data.slot_id_evt = (data >> 22) & 0x1F;
	  if(fadc_data.slot_id_evt != fadc_data.slot_id_hd)
		printf("FADC Warning: event slot id is not the same as the block slot id !\n");
      fadc_data.evt_num_1 = (data & 0x3FFFFF);
      trignum = fadc_data.evt_num_1;
      if( i_print )
        printf("%8X - EVENT HEADER 1 - evt_num = %d\n", data, fadc_data.evt_num_1);
    }
      else
    {
      fadc_data.evt_num_2 = (data & 0x3FFFFF);   // ??? not sure why there is not new type event 2 (for multi blocks?)
      if( i_print )
        printf("%8X - EVENT HEADER 2 - evt_num = %d\n", data, fadc_data.evt_num_2);
    }
      break;
    case 3:     /* TRIGGER TIME */
      if( fadc_data.new_type )
    {
      fadc_data.time_1 = (data & 0xFFFFFF);
      if( i_print )
        printf("%8X - TRIGGER TIME 1 - time = %08x\n", data, fadc_data.time_1);
      fadc_data.time_now = 1;
      time_last = 1;
    }
      else
    {
      if( time_last == 1 )
        {
          fadc_data.time_2 = (data & 0xFFFFFF);
          if( i_print )
        printf("%8X - TRIGGER TIME 2 - time = %08x\n", data, fadc_data.time_2);
          fadc_data.time_now = 2;

		  fadc_trigtime = (fadc_data.time_2 << 24) | fadc_data.time_1; 
        }
	  else
		printf("FADC Warning: trigger time is more than 2 words!! \n");
    }
      break;
    case 4:     /* WINDOW RAW DATA */

      if( fadc_data.new_type )
    {
	  data_type_4 = 1;
      fadc_data.chan = (data & 0x7800000) >> 23;
      fadc_data.width = (data & 0xFFF);
          nsamples = fadc_data.width;

      if(fadc_data.chan!=oldchan){
		nrawdata=0;
		oldchan=fadc_data.chan;
	  }
	  if(fadc_data.chan<FADC_NCHAN){
	     fadc_nhit[fadc_data.chan]++;
	  }
	  else{
        printf("FADC: Something wrong here! chan %d > FADC_NCHAN (%d)\n",fadc_data.chan+1,FADC_NCHAN);
	  }

      if( i_print )
        printf("%8X - WINDOW RAW DATA - chan = %d   nsamples = %d\n",
           data, fadc_data.chan, fadc_data.width);
    }
      else
    {
      fadc_data.valid_1 = 1;
      fadc_data.valid_2 = 1;
      fadc_data.adc_1 = (data & 0x1FFF0000) >> 16;
      if( data & 0x20000000 )
        fadc_data.valid_1 = 0;
      fadc_data.adc_2 = (data & 0x1FFF);
      if( data & 0x2000 )
        fadc_data.valid_2 = 0;
      if( i_print )
        printf("%8X - RAW SAMPLES - valid = %d  chan = %d adc = %4d   valid = %d  adc = %4d\n",
           data, fadc_data.valid_1, fadc_data.chan,
                   fadc_data.adc_1,
           fadc_data.valid_2, fadc_data.adc_2);
          if ((nrawdata < MAXRAW) && (fadc_data.chan < FADC_NCHAN)) {
        frawdata[fadc_data.chan][nrawdata] = fadc_data.adc_1;
            nrawdata++;
        frawdata[fadc_data.chan][nrawdata] = fadc_data.adc_2;
            nrawdata++;
            if (i_print) printf("Found chan %d  data = 0x%x 0x%x  numraw = %d\n",mychan,fadc_data.adc_1,fadc_data.adc_2,nrawdata);
      } else {
            if (nrawdata > MAXRAW) printf("Warning: Decode:  too many raw data words ?\n");
      }
    }
      break;
    case 5:     /* WINDOW SUM */
      fadc_data.over = 0;
      fadc_data.chan = (data & 0x7800000) >> 23;
      fadc_data.adc_sum = (data & 0x3FFFFF);
      if( data & 0x400000 )
    fadc_data.over = 1;
      if( i_print )
    printf("%8X - WINDOW SUM - chan = %d   over = %d   adc_sum = %08x\n",
           data, fadc_data.chan, fadc_data.over, fadc_data.adc_sum);
      break;
    case 6:     /* PULSE RAW DATA */
      if( fadc_data.new_type )
    {
	  data_type_6 = 1;
      fadc_data.chan = (data & 0x7800000) >> 23;
      fadc_data.pulse_num = (data & 0x600000) >> 21;
      fadc_data.thres_bin = (data & 0x3FF);
	  if(fadc_data.chan<FADC_NCHAN){
	     fadc_nhit[fadc_data.chan]++;
	  }
	  else{
        printf("FADC: Something wrong here! chan %d > FADC_NCHAN (%d)\n",fadc_data.chan+1,FADC_NCHAN);
	  }

      if( i_print )
        printf("%8X - PULSE RAW DATA - chan = %d   pulse # = %d   threshold bin = %d\n",
           data, fadc_data.chan, fadc_data.pulse_num, fadc_data.thres_bin);
    }
      else
    {
      fadc_data.valid_1 = 1;
      fadc_data.valid_2 = 1;
      fadc_data.adc_1 = (data & 0x1FFF0000) >> 16;
      if( data & 0x20000000 )
        fadc_data.valid_1 = 0;
      fadc_data.adc_2 = (data & 0x1FFF);
      if( data & 0x2000 )
        fadc_data.valid_2 = 0;
      if( i_print )
        printf("%8X - PULSE RAW SAMPLES - valid = %d  adc = %d   valid = %d  adc = %d\n",
           data, fadc_data.valid_1, fadc_data.adc_1,
           fadc_data.valid_2, fadc_data.adc_2);
    }
      break;
    case 7:     /* PULSE INTEGRAL */
	  data_type_7 = 1;
      fadc_data.chan = (data & 0x7800000) >> 23;
      fadc_data.pulse_num = (data & 0x600000) >> 21;
      fadc_data.quality = (data & 0x180000) >> 19;
      fadc_data.integral = (data & 0x7FFFF);

	  if(fadc_data.chan<FADC_NCHAN){
	     fadc_nhit[fadc_data.chan]++;
		 if(fadc_nhit[fadc_data.chan]==1) fadc_int[fadc_data.chan]=fadc_data.integral;
		 if(fadc_nhit[fadc_data.chan]==2) fadc_int_1[fadc_data.chan]=fadc_data.integral;
		 if(fadc_nhit[fadc_data.chan]>MAXHIT)
		   printf("FADC:  Too many ADC hits (%d hits) in chan %d\n",fadc_nhit[fadc_data.chan],fadc_data.chan);
		 if(fadc_nhit[fadc_data.chan] != ftdc_nhit[fadc_data.chan])
		   printf("FADC:  Warning:  TDC hits %d is not equal to ADC hits %d\n",fadc_nhit[fadc_data.chan],ftdc_nhit[fadc_data.chan]);
	  }
	  else{
        printf("FADC: Something wrong here! ADC chan %d > FADC_NCHAN (%d)\n",fadc_data.chan+1,FADC_NCHAN);
	  }

      if( i_print )
    printf("%8X - PULSE INTEGRAL - chan = %d   pulse # = %d   quality = %d   integral = %d\n",
           data, fadc_data.chan, fadc_data.pulse_num,
           fadc_data.quality, fadc_data.integral);
      break;
    case 8:     /* PULSE TIME */
	  data_type_8 = 1;
      fadc_data.chan = (data & 0x7800000) >> 23;
      fadc_data.pulse_num = (data & 0x600000) >> 21;
      fadc_data.quality = (data & 0x180000) >> 19;
      fadc_data.time = (data & 0xFFFF);

	  if(fadc_data.chan<FADC_NCHAN){
	     ftdc_nhit[fadc_data.chan]++;
		 if(ftdc_nhit[fadc_data.chan]==1) fadc_time[fadc_data.chan]=fadc_data.time;
		 if(ftdc_nhit[fadc_data.chan]==2) fadc_time_1[fadc_data.chan]=fadc_data.time;
		 if(ftdc_nhit[fadc_data.chan]>MAXHIT)
		   printf("FADC:  Too many TDC hits (%d hits) in chan %d\n",ftdc_nhit[fadc_data.chan],fadc_data.chan);
	  }
	  else{
        printf("FADC: Something wrong here! TDC chan %d > FADC_NCHAN (%d)\n",fadc_data.chan+1,FADC_NCHAN);
	  }

      if( i_print )
    printf("%8X - PULSE TIME - chan = %d   pulse # = %d   quality = %d   time = %d\n",
           data, fadc_data.chan, fadc_data.pulse_num,
           fadc_data.quality, fadc_data.time);
      break;
    case 9:     /* STREAMING RAW DATA */
      if( fadc_data.new_type )
    {
      fadc_data.chan_a = (data & 0x3C00000) >> 22;
      fadc_data.source_a = (data & 0x4000000) >> 26;
      fadc_data.chan_b = (data & 0x1E0000) >> 17;
      fadc_data.source_b = (data & 0x200000) >> 21;
      if( i_print )
        printf("%8X - STREAMING RAW DATA - ena A = %d  chan A = %d   ena B = %d  chan B = %d\n",
           data, fadc_data.source_a, fadc_data.chan_a,
           fadc_data.source_b, fadc_data.chan_b);
    }
      else
    {
      fadc_data.valid_1 = 1;
      fadc_data.valid_2 = 1;
      fadc_data.adc_1 = (data & 0x1FFF0000) >> 16;
      if( data & 0x20000000 )
        fadc_data.valid_1 = 0;
      fadc_data.adc_2 = (data & 0x1FFF);
      if( data & 0x2000 )
        fadc_data.valid_2 = 0;
      fadc_data.group = (data & 0x40000000) >> 30;
      if( fadc_data.group )
        {
          if( i_print )
        printf("%8X - RAW SAMPLES B - valid = %d  adc = %d   valid = %d  adc = %d\n",
               data, fadc_data.valid_1, fadc_data.adc_1,
               fadc_data.valid_2, fadc_data.adc_2);
        }
      else
        if( i_print )
          printf("%8X - RAW SAMPLES A - valid = %d  adc = %d   valid = %d  adc = %d\n",
             data, fadc_data.valid_1, fadc_data.adc_1,
             fadc_data.valid_2, fadc_data.adc_2);
    }
      break;
    case 10:        /* PULSE AMPLITUDE DATA */
      fadc_data.chan = (data & 0x7800000) >> 23;
      fadc_data.pulse_num = (data & 0x600000) >> 21;
      fadc_data.vmin = (data & 0x1FF000) >> 12;
      fadc_data.vpeak = (data & 0xFFF);
      if( i_print )
    printf("%8X - PULSE V - chan = %d   pulse # = %d   vmin = %d   vpeak = %d\n",
           data, fadc_data.chan, fadc_data.pulse_num,
           fadc_data.vmin, fadc_data.vpeak);
      break;

    case 11:        /* INTERNAL TRIGGER WORD */
      fadc_data.trig_type_int = data & 0x7;
      fadc_data.trig_state_int = (data & 0x8) >> 3;
      fadc_data.evt_num_int = (data & 0xFFF0) >> 4;
      fadc_data.err_status_int = (data & 0x10000) >> 16;
      if( i_print )
    printf("%8X - INTERNAL TRIGGER - type = %d   state = %d   num = %d   error = %d\n",
           data, fadc_data.trig_type_int, fadc_data.trig_state_int, fadc_data.evt_num_int,
           fadc_data.err_status_int);
      break;
    case 12:        /* UNDEFINED TYPE */
      if( i_print )
    printf("%8X - UNDEFINED TYPE = %d\n", data, fadc_data.type);
      break;
    case 13:        /* END OF EVENT */
      if( i_print )
    printf("%8X - END OF EVENT = %d\n", data, fadc_data.type);
      break;
    case 14:        /* DATA NOT VALID (no data available) */
      if( i_print )
    printf("%8X - DATA NOT VALID = %d\n", data, fadc_data.type);
      break;
    case 15:        /* FILLER WORD */
      if( i_print )
    printf("%8X - FILLER WORD = %d\n", data, fadc_data.type);
      break;
    }

  type_last = fadc_data.type;   /* save type of current data word */


}

