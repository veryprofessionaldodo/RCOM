/*Non-Canonical Input Processing*/

#include "dataLayerWrite.h"

volatile int STOP=FALSE;
volatile int STOP2=FALSE;
int counter = 1;

unsigned char previousSend = NS0;
unsigned char previousRequest = NR1;
unsigned char* previousPacket;
unsigned int previousSize;

void noInformationFrameWrite(int fd, char state, int n) {
    unsigned char toWrite[5];

    // Isn't information frame
    if( state == SET || state == DISC){
        toWrite[0] = FLAG; toWrite[1] = ADDRESS2; toWrite[2] = state; toWrite[3] = state^ADDRESS2; toWrite[4] = FLAG;
        printf("Sent SET or DISC.\n");
    }
    else if(state == UA ){
        toWrite[0] = FLAG; toWrite[1] = ADDRESS1; toWrite[2] = state; toWrite[3] = state^ADDRESS1; toWrite[4] = FLAG;
        printf("Sent UA.\n");
    }

    write(fd, toWrite, sizeof(toWrite));
  }

void processframe(int fd,unsigned  char* buf, unsigned int n) {
    // Check if UA
    if (buf[0] == FLAG && buf[1] == ADDRESS2 && buf[2] == UA
          && buf[3] == (UA^ADDRESS2) && buf[4] == FLAG) {
          printf("Received UA.\n");
	 			  STOP = TRUE;
    } //Check if DISC
    else if (buf[0] == FLAG && buf[1] == ADDRESS1 && buf[2] == DISC
      && buf[3] == (DISC^ADDRESS1) && buf[4] == FLAG) {
          printf("Received DISC.\n");
          noInformationFrameWrite(fd,UA,5);
					STOP2 = TRUE;
    }//Check if RR
    else if(buf[2] == RR || buf[2] == (RR^NR0)){
          printf("Received RR.\n");
          if (previousRequest == buf[2])
            STOP = FALSE;
          else {
            previousRequest = buf[2];
        	  STOP = TRUE;
          }
    }//Check if REJ
		else if(buf[2] == REJ || buf[2] == (REJ^NR0)){
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

 //initialize alarm
	(void) signal(SIGALRM, incCounter);
	alarm(3);

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
	int c = -1;
	alarm(3);
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

    buf = realloc(buf, size + 1); //reallocate memory to join data packet with BCC2 to stuff
    buf[size] = BCC2;
    size++;
    buf = stuff(buf,&size);

    if (previousSend == NS0)
      previousSend = NS1;
    else
      previousSend = NS0;

    unsigned char * packet = (unsigned char *) malloc(size+6);
    packet[0] = FLAG;
    packet[1] = ADDRESS2;
    packet[2] = previousSend;
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
       //printf(" Escrevendo com CN %x ", previousSend );
       //printf("\n");
		   if(write(fd,packet,size + 5) != size + 5 ){
		       printf("Error sending frame\n");
		   };

		   c = frread(fd,buf2,5);
       free(buf2);
	  }
	 counter = 1;
	 free(packet);
	 return c;
}

unsigned char * stuff(unsigned char *buf, unsigned int* size){

  int buf2Index = 0;
  int sizetmp = *size;
  int arraySize = *size;

  unsigned char * buf2 = (unsigned char *) malloc(sizetmp);

  int i;
  for (i = 0; i < sizetmp; i++, buf2Index++) {

    if (buf[i] == 0x7e) { // Needs to be stuffed, it's an escape flag
      arraySize++;
      buf2 = realloc(buf2, arraySize);
      buf2[buf2Index] = 0x7d;
      buf2[buf2Index+1] = 0x5e;
      buf2Index++;
    }
    else if (buf[i] == 0x7d) { // Needs to be stuffed, it's an escape flag
      arraySize++;
      buf2 = realloc(buf2, arraySize);
      buf2[buf2Index] = 0x7d;
      buf2[buf2Index+1] = 0x5d;
      buf2Index++;
    }
    else {
      buf2[buf2Index] = buf[i];
    }

  }

  *size = arraySize;
  return buf2;
}
