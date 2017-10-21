/*Non-Canonical Input Processing*/

#include "dataLayerWrite.h"

volatile int STOP=FALSE;
int current_state, previous_state;
int counter = 0;
int serialPortFD;
//struct linkLayer l*;
int CN = 0;


void noInformationFrameWrite(int fd, char state, int n) {
    unsigned char toWrite[5];

        // Isn't information frame
        if( state == SET || state == DISC){
            toWrite[0] = FLAG; toWrite[1] = 0x03; toWrite[2] = state; toWrite[3] = state^0x03; toWrite[4] = FLAG;}
        else if(state == UA ){
            toWrite[0] = FLAG; toWrite[1] = 0x01; toWrite[2] = state; toWrite[3] = state^0x01; toWrite[4] = FLAG;}

	    write(fd, toWrite, sizeof(toWrite));

}

void processframe(int fd, char* buf, int n) {
	alarm(0);
    printf("entrou no process frame\n");
    printf(" buf[0] : %d n : %d \n",buf[2], n);
    if (n == 5) {
        // Check if UA
        if (buf[0] == FLAG && buf[1] == 0x03 && buf[2] == UA
            && buf[3] == UA^0x03 && buf[4] == FLAG) {
	 	 			STOP = TRUE;
        }
        else if (buf[0] == FLAG && buf[1] == 0x01 && buf[2] == DISC
            && buf[3] == DISC^0x01 && buf[4] == FLAG) {

                 noInformationFrameWrite(fd,UA,5);
				 			 	 STOP = TRUE;
        }
    }
    else {

    }

}

int frread(int fd, unsigned char * buf, int maxlen) {
    int n=0;
    int ch;

		//printf("fd : %d ",fd);
    while(1) {
        if((ch = read(fd, buf + n, 1)) < 0) {
					//printf(" ch %d\n", ch);
            return -1; // ERROR
           }
        printf("ceasdas %d\n", (int) buf[n]);
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

	 counter = 0;
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
	 counter = 0;
	 return c;
}

int llwrite(int fd, unsigned char* buf, int size){
printf("Entrei no llwrite \n");

// Xor BCC2, stuff, fazer trama
unsigned int i = 0;
unsigned char BCC2 = 0;
for(i = 0; i < size-1;i++){
	BCC2 = BCC2^buf[i];
}

/*buf = realloc(buf,size*sizeof(char) + sizeof(BCC2));

buf[size] = BCC2;*/

stuff(buf,size);

unsigned char* packet = malloc(size+ 5);
packet[0] = FLAG;
packet[1] = 0x03;
packet[2] = CN;
packet[3] = packet[1]^packet[2];

for(i = 0; i < size;i++){
	packet[i+4] = buf[i];
}
packet[size + 4] = BCC2;
packet[size + 5] = FLAG;

if(write(fd,packet,size+ 5 ) != size+ 5 ){
	printf("Error sending frame\n");
}
	return 0;
}

unsigned int stuff(unsigned char *buf, unsigned int size){

	int i;
	for (i = 0; i < size; i++) {
		if (buf[i] == 0x7e) { // Needs to be stuffed, it's an escape flag
			realloc(buf, size + sizeof(unsigned char*));
			memmove(buf + i + 1, buf + i, size- i);
			buf[i] = 0x7d;
			buf[i+1] = 0x5e;
		}
		if (buf[i] == 0x7d) { // Needs to be stuffed, it's an escape flag
			realloc(buf, size + sizeof(unsigned char*));
			memmove(buf + i + 1, buf + i, size- i);
			buf[i] = 0x7d;
			buf[i+1] = 0x5d;
		}

	}
	printf("%s\n",buf );

return sizeof(buf);
}
