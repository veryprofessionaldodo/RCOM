/*Non-Canonical Input Processing*/

#include "dataLayerRead.h"

#define FLAG 0x7e
#define SET 0x03
#define DISC 0x0B
#define UA 0x07
#define RR 0x05
#define REJ 0x01
#define NS0 0x40
#define NS1 0x00
#define NR0 0x80
#define NR1 0x00
#define CONTROL_PACKET_START 0x02
#define CONTROL_PACKET_END 0x03


FILE * file;
volatile int STOP=FALSE;

unsigned char* filename;
int filesize;

void frwrite(int fd, char state, char NR) {
	unsigned char toWrite[5];

		// Isn't information frame
	if (state == UA) {
	    printf("mandei um UA para o fd %d\n", fd);
    	toWrite[0] = FLAG; toWrite[1] = 0x03; toWrite[2] = state; toWrite[3] = state^0x03; toWrite[4] = FLAG;
	}
	else if (state == DISC) {
  	printf("mandei um DISC para o fd %d\n", fd);
		toWrite[0] = FLAG; toWrite[1] = 0x01; toWrite[2] = state; toWrite[3] = state^0x01; toWrite[4] = FLAG;
	}
  else if (state == RR || state == REJ) {
    toWrite[0] = FLAG; toWrite[1] = 0x01; toWrite[2] = state^NR; toWrite[3] = state^0x01; toWrite[4] = FLAG;
  }

	write(fd, toWrite, sizeof(toWrite));
}


int processframe(int fd, char* buf, int n) {

  char r = 0x00;

  if (buf[2] == NS0)
    r = NR1;
  else
    r = NR0;

  if (hasErrors(buf)) {
      frwrite(fd, REJ, r);
      return -1;
  }

  printf("n %i\n", n);
	if (n == 5) {
		// Check if SET
		if (buf[0] == FLAG && buf[1] == 0x03 && buf[2] == SET
			&& buf[3] == SET^0x03 && buf[4] == FLAG) {
	        printf("recebi um set\n");
	        STOP = TRUE;
			return;
		}
		// Check if DISC
		else if (buf[0] == FLAG && buf[1] == 0x03 && buf[2] == DISC
			&& buf[3] == DISC^0x03 && buf[4] == FLAG) {
           printf("recebi um disc\n");
           STOP = TRUE;
           return;
		}

		// Check if UA
		else if (buf[0] == FLAG && buf[1] == 0x01 && buf[2] == UA
			&& buf[3] == DISC^0x01 && buf[4] == FLAG) {
			 STOP = TRUE;
			return;
		}
	}
	else {
		processInformationFrame(fd, buf);
	}

	return 0;

}

void processInformationFrame(int fd, unsigned char* buf) {
	// I Need to see if it's a control or data packet
  printf("estou a tentar processar informação, olhem para mim que hacker que sou\n");
  char r;
  if (buf[2] == NS0)
    r = NR1;
  else
    r = NR0;

	// Control Start
	if (buf[4] == CONTROL_PACKET_START) { // We chose the first T to be filename, and the second to be size
    printf("pressupostamente estou a ler um start\n");
    // Filename

	    int nameSize = buf[6];
	
		filename = (unsigned char *) malloc(nameSize * sizeof(unsigned char*));
		
		int x;
		for (x = 0; x < nameSize; x++) {
			filename[x] = buf[x+7];
			}
    	
		printf("buf[7] %c\n", buf[7]);
		printf("buf[7] %c\n", buf[8]);
			printf("buf[7] %c\n", buf[9]);
		printf("buf[7] %c\n", buf[10]);
		printf("buf[7] %c\n", buf[11]);
		printf("buf[7] %c\n", buf[12]);
		printf("buf[7] %c\n", buf[13]);
			printf("buf[7] %c\n", buf[14]);
	
	    int nextPos = nameSize * sizeof(char);
		int fileInformationSize = (int)strtol(&buf[nextPos+1], NULL, 0);
  		char fileBuffer[fileInformationSize];
	
		memcpy(fileBuffer, buf+ nextPos+2, fileInformationSize * sizeof(char));
	
	    printf("filename %s\n", filename);
	
	    FILE *fp = fopen(filename, "w");
	    if (fp == NULL)
	      printf("rica merda cvaralho\n");
	    else
	      fclose(fp);

    //printf("%d \n",sizeof(filename));
	}
	// Control End
	else if (buf[4] == CONTROL_PACKET_END) {
		printf("pressupostamente estou a ler um end\n");
		STOP = TRUE;
	}
	  // Data Packet
  else {
    	// Write on the file data size

  }
  
  frwrite(fd, RR, r);
}

int frread(int fd, unsigned char * buf2, int maxlen) {

	int n=0;
	int ch;
    char buf[255];
	while(1) {
 		if((ch= read(fd, buf + n, 1)) <= 0) {
			return ch; // ERROR
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
			return 0;
		}
	}
	return 0;	
}

int llopen(int fd) {
  int res;

  printf("entrei no open\n");

  while (STOP==FALSE) {
    char buf[255]; 	
    printf("merda %d\n", STOP);
	frread(fd,buf,sizeof(buf));  
	printf("merda %d\n", STOP); 	
  }
  printf("Sai daqui, pimbas lá na cena\n");
  frwrite(fd, UA, 0x00);

  STOP = FALSE;
  return 0;
}

int llread(int fd) {
  int res;
  printf("entrei no read\n");

  while (STOP==FALSE) {
    char buf[255];  
    frread(fd,buf,sizeof(buf));  
  }

  STOP = FALSE;
  return 0;
}

int llclose(int fd) {
  int res;
  printf("entrei no llclose\n");

  while (STOP==FALSE) {
    char buf[255];  
    frread(fd,buf,sizeof(buf));
  }
  frwrite(fd, DISC, 0x00);
  return 0;
}


int hasErrors(char * buf) {
  // See if frame has any errors
  return FALSE;
}

unsigned char* destuff(unsigned char * buf) {
  int i;

  for (i = 0; i < sizeof(buf) - 1 * sizeof(unsigned char*); i++) {
    if (buf[i] == 0x7d && buf[i+1] == 0x5e) {
      buf[i] = 0x7e;
      buf = memmove(buf + i, buf + 2, sizeof(buf)- (i+1)*sizeof(unsigned char*));
      realloc(buf, sizeof(buf)-sizeof(unsigned char*));
    }
    if (buf[i] == 0x7d && buf[i+1] == 0x5d) {
      buf[i] = 0x7d;
      memmove(buf + i, buf + 2, sizeof(buf)- (i+1)*sizeof(unsigned char*));
      buf = realloc(buf, sizeof(buf)-sizeof(unsigned char*));
    }
  }

  return buf;
}
