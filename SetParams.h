#define MAX_BLOCK_SIZE 20
#define MAX_ROCS       2
#define VTP_ROC        5      //VTP ROC ID
#define TI_ROC         1      //Physics modules (FADC, VETROC, Scaler) ROC ID
#define FIRSTWORD_VETROC 0xb0b0b0b4
#define FIRSTWORD_FADC   0xb0b0b0b5
#define FIRSTWORD_SCALER 0xb0b0b0b6

#define NBANK          3
#define FADC_BANK      3
#define FADC_SLOT      3
#define VTP_BANK       0x56
#define VTP_SLOT       11 


const int FADC_NCHAN=16;        // FADC maximum number of channels
const int MAXRAW=33;	       // FADC raw mode maximum window width
const int NCLUST=10;	       // maximum number of clusters per trigger
