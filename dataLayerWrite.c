/*Non-Canonical Input Processing*/

#include "dataLayerWrite.h"

volatile int STOP=FALSE;
int current_state, previous_state;
int counter = 1;
int serialPortFD;
int CN = 0x00;


void noInformationFrameWrite(int fd, char state, int n) {
    unsigned char toWrite[5];

        // Isn't information frame
        if( state == SET || state == DISC){
            toWrite[0] = FLAG; toWrite[1] = 0x03; toWrite[2] = state; toWrite[3] = state^0x03; toWrite[4] = FLAG;
          printf("mandei set\n");}
        else if(state == UA ){
            toWrite[0] = FLAG; toWrite[1] = 0x01; toWrite[2] = state; toWrite[3] = state^0x01; toWrite[4] = FLAG;}

	    write(fd, toWrite, sizeof(toWrite));

}

void processframe(int fd, char* buf, unsigned int n) {
	alarm(0);
    printf("entrou no process frame\n");
    // Check if UA
        if (buf[0] == FLAG && buf[1] == 0x03 && buf[2] == UA
            && buf[3] == UA^0x03 && buf[4] == FLAG) {
              printf("recebi um ua\n");
	 	 			STOP = TRUE;
        }
        else if (buf[0] == FLAG && buf[1] == 0x01 && buf[2] == DISC
            && buf[3] == DISC^0x01 && buf[4] == FLAG) {
              printf("recebi um disc\n");
              noInformationFrameWrite(fd,UA,5);
				 			STOP = TRUE;
        }
        else if(buf[2] == RR || buf[2] == RR^0x40){
          printf("recebi um rr\n");
			STOP = TRUE;
        }
		else if(buf[2] == REJ || buf[2] == REJ^0x40){
      printf("recebi um rej\n");
			STOP = FALSE;
        }
		else { // ERROR

		}


}


int frread(int fd, unsigned char * buf, int maxlen) {
    int n=0;
    int ch;

		//printf("fd : %d ",fd);
    while((ch = read(fd, buf + n, 1))) {
        if(ch < 0) {
					//printf(" ch %d\n", ch);
            return -1; // ERROR
           }
        //printf("ceasdas %d\n", (int) buf[n]);
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
	printf("Entrei no llopen\n");
	int c = -1;
	char buf[255];
	serialPortFD = fd;
	(void) signal(SIGALRM, incCounter);
	alarm(3);
	previous_state = CONNECTING;

  while(counter < 3 && STOP == FALSE){
		 noInformationFrameWrite(fd,SET, 5);
		 c = frread(fd,buf,5);
	 }

	 counter = 1;
	 STOP = FALSE;
	 return c;
}

int llclose(int fd){
	printf("Entrou no llclose \n" );
	STOP = FALSE;
	int c = -1;
	char buf[255];
	alarm(3);
	previous_state = CONNECTING;
	while(counter < 3 && STOP==FALSE){
		 noInformationFrameWrite(fd,DISC, 5);
		 c = frread(fd,buf,5);
  }
	 counter = 1;
	 return c;
}

int llwrite(int fd, unsigned char* buf, int size){
  printf("Entrei no llwrite com size %d\n", size);

  // Xor BCC2, stuff, fazer trama
  unsigned int i = 0;
  unsigned char BCC2 = 0x00;
  for(i = 0; i < size;i++){
	   BCC2 = BCC2^buf[i];
   }

   stuff(buf,&size);
   unsigned char * packet = (unsigned char *) malloc(size+6);
   packet[0] = FLAG;
   packet[1] = 0x03;
   packet[2] = CN;
   packet[3] = packet[1]^packet[2];

   for(i = 0; i < size;i++){
	    packet[i+4] = buf[i];
    }
    packet[size + 4] = BCC2;
    packet[size + 5] = FLAG;

    //waiting for response;
	  STOP = FALSE;
	  int c = -1;
	  char buf2[255];
	  alarm(3);

    int x;

    /*for(x = 0; x < size + 6 ; x++) {
		   printf(" [%d] = %x ", x, packet[x]);
	  }*/
    printf("\n");

    while(counter < 3 && STOP == FALSE){
		  if(write(fd,packet,size + 10 ) != size + 6 ){
		     printf("Error sending frame\n");
		  };
         //  sleep(1);
		  c = frread(fd,buf2,5);
	  }
	 counter = 1;

	 if(CN = 0x00)
	   CN = 0x40;
		 else
		 CN = 0x00;

     free(packet);

	 return c;
}

void stuff(unsigned char *buf, unsigned int* size){

  int sizetmp = *size;
	int i;
	for (i = 0; i < sizetmp; i++) {
		if (buf[i] == 0x7e) { // Needs to be stuffed, it's an escape flag
			buf = realloc(buf, sizetmp + sizeof(unsigned char*));
			memmove(buf + i + 1, buf + i, sizetmp- i);
			buf[i] = 0x7d;
			buf[i+1] = 0x5e;
      sizetmp++;
		}
		if (buf[i] == 0x7d) { // Needs to be stuffed, it's an escape flag
			buf = realloc(buf, sizetmp + sizeof(unsigned char*));
			memmove(buf + i + 1, buf + i, sizetmp- i);
			buf[i] = 0x7d;
			buf[i+1] = 0x5d;
      sizetmp++;
		}

	}
  *size = sizetmp;
}
