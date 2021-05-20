struct fadc_pulse{
  int npulse;
  int nsample[3];
};

struct fadc_pulse FindPulses(int rawADC[],int width, int NPED){

     fadc_pulse pulse;
     pulse.npulse=0;
     pulse.nsample[0]=0;
     pulse.nsample[1]=0;
     pulse.nsample[2]=0;

     bool rising=false;

     for(int ii=0; ii<width; ii++){
	if(rawADC[ii]>NPED){
	  if(rising) pulse.nsample[pulse.npulse-1]++;
	  else{
	     rising=true;
	     pulse.npulse++;
	  }
       
	}
	else{
	   if(rising) rising=false;
	}
     }

     return pulse;


}
