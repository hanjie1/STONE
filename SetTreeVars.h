//global variables for tree

/*** TI tree variables ***/
int tHelicity;  //TS6 bit, helicity level (inverted)
int tMPS;      //TS5 bit, TSettle level
int evtype;    // event type;
ULong64_t ti_timestamp;  //time stamp from TI


/***  FADC tree varibles  ***/
int fadc_mode;                        // FADC mode
ULong64_t fadc_trigtime;              // FADC trigger time 
Int_t fadc_int[FADC_NCHAN];           // ADC integral for the first hit per channel
Int_t fadc_time[FADC_NCHAN];          // pulse time for the first hit per channel
Int_t fadc_int_1[FADC_NCHAN];         // ADC integral for the second hit per channel 
Int_t fadc_time_1[FADC_NCHAN];        // pulse time for the second hit per channel
Int_t fadc_nhit[FADC_NCHAN];          // number of hits per channel
Int_t fadc_scal_cnt[16];	      // FADC scaler counts per channel
Int_t fadc_scal_time;		      // FADC scaler timer
Int_t fadc_scal_trigcnt;	      // FADC scaler trigger count
Int_t frawdata[FADC_NCHAN][MAXRAW];   // raw mode ADC samples

Int_t fadc_scal_update;		      // flag showing if the FADC scaler is updated
Int_t fadc_scal_precnt[16];	      // previous FADC scaler counts per channel
Int_t fadc_scal_pretime;	      // previoud FADC scaler timer
Int_t fadc_scal_rate[16];	      // the events rate per channel calculated from the scaler counts and timer

/***  VTP tree variables ***/
ULong64_t vtp_trigtime;                          // vtp trigger time
Int_t vtp_fadc_scalcnt[FADC_NCHAN];   // vtp helicity based scaler fadc counts
Int_t trigcnt[5];                     // vtp helicity based trigger counts
Int_t busytime;                       // busy time
Int_t livetime;                       // live time
Int_t hel_win_cnt_1;                  // helicity window counts in helicity based trigger scalers
Int_t trig_pattern[64];               // trigger bit pattern
Int_t trig_pattern_time[64];          // trigger bit pattern time
Int_t pattern_num;                    // number of trigger bit patterns
Int_t last_mps_time;                  // last MPS time for the past helicity
Int_t vtp_past_hel[6];                // last 173 helicity windows
Int_t hel_win_cnt;                    // helicity window counts in past helicity 
Int_t current_helicity;               // helicty after the delay is removed
Int_t vtp_helicity;                   // the most recent helicity in vtp_past_hel[0] bit 0 (inverted)


