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

volatile int STOP=FALSE;

FILE * file;
unsigned int filesize;
unsigned int receivedData;

int frread(int fd, unsigned char * buf, int maxlen) {
	int n=0;
	int ch;

	while(1) {
 		if((ch= read(fd, buf + n, 1)) <= 0) {
			return ch; // ERROR
		}

    printf("Ler %x  n%d\n", buf[n], n);

		if(n==0 && buf[n] != FLAG)
			continue;

		if(n==1 && buf[n] == FLAG)
			continue;

		n++;

		if(buf[n-1] != FLAG && n == maxlen) {
			printf("piças\n");
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

int hasErrors(char * buf) {
  // See if frame has any errors
  return FALSE;
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

	if (n == 5) {
		// Check if SET
		if (buf[0] == FLAG && buf[1] == 0x03 && buf[2] == SET
			&& buf[3] == SET^0x03 && buf[4] == FLAG) {
	        printf("recebi um set\n");
	        STOP = TRUE;
			return 0;
		}
		// Check if DISC
		else if (buf[0] == FLAG && buf[1] == 0x03 && buf[2] == DISC
			&& buf[3] == DISC^0x03 && buf[4] == FLAG) {
           printf("recebi um disc\n");
           STOP = TRUE;
           return 0;
		}

		// Check if UA
		else if (buf[0] == FLAG && buf[1] == 0x01 && buf[2] == UA
			&& buf[3] == DISC^0x01 && buf[4] == FLAG) {
			 STOP = TRUE;
			 return 0;
		}
	}
	else {
		processInformationFrame(fd, buf, n);
	}

	return 0;

}

void processInformationFrame(int fd, unsigned char* buf, int n) {
	printf("lendo informacao\n");
	char r;
  if (buf[2] == NS0)
    r = NR1;
  else
    r = NR0;

		int i;

		/*for (i = 0 ; i < n; i++) {
			printf("buf %d %x\n",i, buf[i]);
		}*/

	// Control Start
	if (buf[4] == CONTROL_PACKET_START) { // We chose the first T to be filename, and the second to be size
    printf("pressupostamente estou a ler um start\n");
    // Filename
		unsigned char* filename;

    int nameSize = buf[6];

		filename = (unsigned char *) malloc(nameSize * sizeof(unsigned char*));

		int x;
		for (x = 0; x < nameSize; x++) {
			filename[x] = buf[x+7];
		}


	  int nextPos = nameSize * sizeof(char);
		unsigned int fileInformationSize = (unsigned int) buf[nextPos+1];


		// printf("fileinformationsize %x\n", fileInformationSize);

		unsigned char *filesizeChar = (unsigned char *) malloc(fileInformationSize * sizeof(unsigned char*));

		for (x = 0; x < 4; x++) {
			filesizeChar[x] = buf[x+nextPos+9];
			printf("buf merda %x\n", buf[x+nextPos+9]);
		}

		/*filesizeChar[0] = (filesize>>24) & 0xFF;
		filesizeChar[1] = (filesize>>16) & 0xFF;
		filesizeChar[2] = (filesize>>8) & 0xFF;
		filesizeChar[3] = filesize & 0xFF;*/

		filesize = *(int *)filesizeChar;

		printf("filesize %d\n", filesize);

	  file = fopen(filename, "w");
		if (file == NULL)
			printf("Error opening file!\n");

		printf("pressupostamente acabei de ler um start\n");
		frwrite(fd, RR, r);
	}

	// Control End
	else if (buf[4] == CONTROL_PACKET_END) {
		printf("pressupostamente estou a ler um end\n");
		frwrite(fd, RR, r);
	}


	  // Data Packet
  else if (buf[4] == 0x01) {
		printf("pressupostamente estou a ler dados mesmo\n");
		int numPackets = buf[6]*256 + buf[7];

		int i;

		for (i = 0 ; i < numPackets; i++ ) {
			fwrite(&buf[8+i], 1, sizeof(buf[8+i]), file);
		}

		frwrite(fd, RR, r);
  }

	else { // Not a valid packet
		frwrite(fd, REJ, r);
	}
}

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

int llopen(int fd) {
  int res;

  while (STOP==FALSE) {
		unsigned char* buf = (unsigned char*) malloc(sizeof(unsigned char*)*1000);
    frread(fd,buf,10000);
		free(buf);
  }

  frwrite(fd, UA, 0x00);

  STOP = FALSE;
  return 0;
}

int llread(int fd) {
  int res;

  while (STOP==FALSE) {
    unsigned char* buf = (unsigned char*) malloc(sizeof(unsigned char*)*1200);
    frread(fd,buf,10000);
		free(buf);
  }

  STOP = FALSE;
  return 0;
}

int llclose(int fd) {
  int res;
  printf("entrei no llclose\n");
	frwrite(fd, DISC, 0x00);

  while (STOP==FALSE) {
		unsigned char* buf = (unsigned char*) malloc(sizeof(unsigned char*)*255);
    frread(fd,buf,10000);
		free(buf);
  }
  return 0;
}

unsigned char* destuff(unsigned char * buf) {
  int i;

  for (i = 0; i < sizeof(buf) - 1 * sizeof(unsigned char*); i++) {
    if (buf[i] == 0x7d && buf[i+1] == 0x5e) {
      buf[i] = 0x7e;
      buf = memmove(buf + i, buf + 2, sizeof(buf)- (i+1)*sizeof(unsigned char*));
      buf = realloc(buf, sizeof(buf)-sizeof(unsigned char*));
			if (buf == NULL)
				printf("Realloc failure!\n");

    }
    if (buf[i] == 0x7d && buf[i+1] == 0x5d) {
      buf[i] = 0x7d;
      memmove(buf + i, buf + 2, sizeof(buf)- (i+1)*sizeof(unsigned char*));
      buf = realloc(buf, sizeof(buf)-sizeof(unsigned char*));
			if (buf == NULL)
				printf("Realloc failure!\n");
    }
  }

  return buf;
}
