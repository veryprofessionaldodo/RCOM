/*Non-Canonical Input Processing*/

#include "dataLayerWrite.h"

volatile int STOP=FALSE;
volatile int STOP2=FALSE;
int current_state, previous_state;
int counter = 1;
int serialPortFD;
int CN = 0x00;

unsigned char* previousFrameSent;
unsigned int previousFrameSize;

void noInformationFrameWrite(int fd, char state, int n) {
    unsigned char toWrite[5];

        // Isn't information frame
        if( state == SET || state == DISC){
            toWrite[0] = FLAG; toWrite[1] = 0x03; toWrite[2] = state; toWrite[3] = state^0x03; toWrite[4] = FLAG;
            printf("Sent SET or DISC.\n");
        }
        else if(state == UA ){
            toWrite[0] = FLAG; toWrite[1] = 0x01; toWrite[2] = state; toWrite[3] = state^0x01; toWrite[4] = FLAG;
            printf("Sent UA.\n");
        }

        previousFrameSent = toWrite;
        previousFrameSize = 5;
	      write(fd, toWrite, sizeof(toWrite));
  }

void processframe(int fd,unsigned  char* buf, unsigned int n) {
	  //alarm(0);
    // Check if UA
    if (buf[0] == FLAG && buf[1] == 0x03 && buf[2] == UA
          && buf[3] == (UA^0x03) && buf[4] == FLAG) {
          printf("Received UA.\n");
	 			  STOP = TRUE;
    }
    else if (buf[0] == FLAG && buf[1] == 0x01 && buf[2] == DISC
      && buf[3] == (DISC^0x01) && buf[4] == FLAG) {
          printf("Received DISC.\n");
          noInformationFrameWrite(fd,UA,5);
					STOP2 = TRUE;
    }
    else if(buf[2] == RR || buf[2] == (RR^0x80)){
          printf("Received RR.\n");
		    	STOP = TRUE;
    }
		else if(buf[2] == REJ || buf[2] == (REJ^0x80)){
          printf("Received REJ, resending.\n");
          STOP = FALSE;
    }
		else { // ERROR
          printf("Received ERROR.\n");
		}
}


int frread(int fd, unsigned char * buf, int maxlen) {
    int n=0;
    int ch;

    while((ch = read(fd, buf + n, 1))) {
        if(ch < 0) {
            return -1; // ERROR
           }

        if(n==0 && buf[n] != FLAG)
            continue;

        if(n==1 && buf[n] == FLAG)
            continue;

        n++;

        if(buf[n-1] != FLAG && n == maxlen) {
            n=0;
            continue;
        }

        if(buf[n-1] == FLAG && n > 4) {
            processframe(fd, buf, n);
            return n;
        }
    }

}

void incCounter(){
	counter++;
	if (counter >= 3) {
		printf("Timed Out!\n");
		exit(-1);
	}
	alarm(3);
}

int llopen(int fd){
	printf("Entered llopen.\n");
	int c = -1;
	serialPortFD = fd;
	(void) signal(SIGALRM, incCounter);
	alarm(3);
	previous_state = CONNECTING;

  while(counter < 3 && STOP == FALSE){
    unsigned char * buf = (unsigned char *)malloc(255);
		 noInformationFrameWrite(fd,SET, 5);
		 c = frread(fd,buf,5);
     free(buf);
	 }

	 counter = 1;
	 STOP = FALSE;
	 return c;
}

int llclose(int fd){
	printf("Entered llclose. \n" );
	STOP = FALSE;
	int c = -1;
	alarm(3);
	previous_state = CONNECTING;
	while(counter < 3 && STOP2==FALSE){
    unsigned char * buf = (unsigned char *)malloc(255);
		 noInformationFrameWrite(fd,DISC, 5);
		 c = frread(fd,buf,5);
     free(buf);
  }
	 counter = 1;
	 return c;
}

int llwrite(int fd, unsigned char* buf, int size){
  printf("Entered llwrite.\n");

  // Xor BCC2, stuff, fazer trama
    unsigned int i = 0;
    unsigned char BCC2 = 0x00;
    for(i = 0; i < size;i++){
	     BCC2 = (BCC2^buf[i]);
    }
    buf = realloc(buf, size + 1);
    printf("BCC2 %x\n", BCC2);
    buf[size] = BCC2;
    size++;
    buf = stuff(buf,&size);


    /*for(i= 0; i < size; i++) {
      printf("buf[%d] %x\n", i, buf[i]);
    }*/

    int x;

    unsigned char * packet = (unsigned char *) malloc(size+6);
    packet[0] = FLAG;
    packet[1] = 0x03;
    packet[2] = CN;
    packet[3] = packet[1]^packet[2];

    for(i = 0; i < size;i++){
	     packet[i+4] = buf[i];
     }

    packet[size + 4] = FLAG;

    free(buf);

    //waiting for response;
	   STOP = FALSE;
     int c = -1;


     while(counter < 3 && STOP == FALSE){
       unsigned char * buf2 = (unsigned char *)malloc(255);
       alarm(3);
       previousFrameSent = packet;
       previousFrameSize = size + 5;
       int z;
       /*for (z = 0; z < size+5; z++) {
         printf(" packet[%d] %x\n ", z, packet[z]);
       }*/
       printf("\n");
		   if(write(fd,packet,size + 5) != size + 5 ){
		       printf("Error sending frame\n");
		   };

		   c = frread(fd,buf2,5);
       free(buf2);
	  }
	 counter = 1;

	 if(CN = 0x00)
	   CN = 0x40;
		 else
		 CN = 0x00;

     free(packet);

	 return c;
}

unsigned char * stuff(unsigned char *buf, unsigned int* size){

  int buf2Index = 0;
  int sizetmp = *size;
  int arraySize = *size;
  int z;

  unsigned char * buf2 = (unsigned char *) malloc(sizetmp);
  //buf2 = buf;

  int i;
  for (i = 0; i < sizetmp; i++, buf2Index++) {

    if (buf[i] == 0x7e) { // Needs to be stuffed, it's an escape flag
      arraySize++;
      //printf(" stuff1 sizetmp %d ", sizetmp);
      buf2 = realloc(buf2, arraySize);
      buf2[buf2Index] = 0x7d;
      buf2[buf2Index+1] = 0x5e;
      buf2Index++;
    }
    else if (buf[i] == 0x7d) { // Needs to be stuffed, it's an escape flag
      arraySize++;
      //printf(" stuff2 sizetmp %d ", sizetmp);
      buf2 = realloc(buf2, arraySize);
      buf2[buf2Index] = 0x7d;
      buf2[buf2Index+1] = 0x5d;
      buf2Index++;
    }
    else {
      buf2[buf2Index] = buf[i];
    }

  }

  /*for (z = 0; z < buf2Index; z++) {
    printf("pos[%d] %x\n", z, buf2[z]);
  }*/
  *size = arraySize;
  return buf2;
}
