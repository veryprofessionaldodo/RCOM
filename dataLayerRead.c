/*Non-Canonical Input Processing*/

#include "dataLayerRead.h"

volatile int STOP=FALSE;

FILE* file;
unsigned int filesize;
unsigned int receivedData = 0;

int frread(int fd, unsigned char * buf, int maxlen) {
	int n=0;
	int ch;

	while(1) {
 		if((ch= read(fd, buf + n, 1)) <= 0) {
			return ch; // ERROR
		}

		if(n==0 && buf[n] != FLAG)
			continue;

		if(n==1 && buf[n] == FLAG)
			continue;

		n++;

		//printf("ler %x n %d\n", buf[n-1], n);

		if(buf[n-1] != FLAG && n == maxlen) {
			n=0;
			continue;
		}

		if(buf[n-1] == FLAG && n > 4) {

 			processframe(fd, buf, n);
			return 0;
		}
	}
	printf("\n");
	return 0;
}

int hasErrors(unsigned char * buf, int size) {
  if (size == 5) {
		if (buf[2] == SET || buf[2] == DISC) {
				if ((buf[2]^0x03) != buf[3]) {
						//  buf[2] == SET	&& buf[3] == SET^0x03
					printf("ERROR ON SET or DISC!\n");
					return TRUE;
				}
				else
					return FALSE;
		}
		else {
			if (buf[2] == UA) {
				if ((buf[2]^0x01) != buf[3]) {
					printf("ERROR ON UA!\n");
					return TRUE;
				}
			}
			else
				return FALSE;
		}

	}

	else {
			unsigned char BCC2 = 0x00;

			if ((buf[2]^0x03) != buf[3]) {
				printf("ERROR ON Information Header!\n");

				return TRUE;
			}
			else { // Check errors in message
					int j;
					for (j = 4; j < size-1;j++) {
						BCC2 = BCC2^buf[j];
					}
					if (BCC2 != buf[size-2]){
						printf("ERROR ON Information Frame!\n");

						int z;

						for (z = 0; z< size; z++) {
							printf("buf[%d] %x\n", z, buf[z]);
						}
						return TRUE;
					}
					else {
						return FALSE;
					}
			}
	}
	return FALSE;
}

int processframe(int fd, unsigned char* buf, int n) {
  char r = 0x00;

  if (buf[2] == NS0)
    r = NR1;
  else
    r = NR0;


	if (n == 5) {
	  if (hasErrors(buf,5)) {
	      frwrite(fd, REJ, r);
	      return -1;
	  }
		// Check if SET
		if (buf[0] == FLAG && buf[1] == 0x03 && buf[2] == SET
			&& buf[3] == SET^0x03 && buf[4] == FLAG) {
	        printf("Received SET.\n");
	        STOP = TRUE;
			return 0;
		}
		// Check if DISC
		else if (buf[0] == FLAG && buf[1] == 0x03 && buf[2] == DISC
			&& buf[3] == DISC^0x03 && buf[4] == FLAG) {
           printf("Received DISC.\n");
           STOP = TRUE;
           return 0;
		}

		// Check if UA
		else if (buf[0] == FLAG && buf[1] == 0x01 && buf[2] == UA
			&& buf[3] == DISC^0x01 && buf[4] == FLAG) {
			 printf("Received UA.\n");
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
	char r;
  if (buf[2] == NS0)
    r = NR1;
  else
    r = NR0;

	int i = n;

	buf = destuff(buf, &i);

   if (hasErrors(buf,i)) {
	      frwrite(fd, REJ, r);
	      return;
   }

	// Control Start
	if (buf[4] == CONTROL_PACKET_START) { // We chose the first T to be filename, and the second to be size
		printf("Reading Control Packet Start.\n");
    	unsigned char* filename;

    	int nameSize = buf[6];

		filename = (unsigned char *) malloc(nameSize * sizeof(unsigned char*));

		int x;
		for (x = 0; x < nameSize; x++) {
			filename[x] = buf[x+7];
		}

	  	int nextPos = nameSize * sizeof(char);
		unsigned int fileInformationSize = (unsigned int) buf[nextPos+1];

		unsigned char *filesizeChar = (unsigned char *) malloc(fileInformationSize * sizeof(unsigned char*));

		for (x = 0; x < 4; x++) {
			filesizeChar[x] = buf[x+nextPos+9];
		}

		filesize = *(int *)filesizeChar;

	  	file = fopen(filename, "wb");
		if (file == NULL)
			printf("Error opening file!\n");

		frwrite(fd, RR, r);
	}

	// Control End
	else if (buf[4] == CONTROL_PACKET_END) {
		printf("Reading Control Packet End.\n");
		fclose(file);
		frwrite(fd, RR, r);
	}


	  // Data Packet
  	else if (buf[4] == 0x01) {
		int i;
		unsigned char * buftmp = (unsigned char*)malloc(n-5);

		memcpy(buftmp, buf + 4, n-5);

		int sizeOfBuftmp = n-5;

    	buftmp = destuff(buftmp,&sizeOfBuftmp);

		int numPackets = buftmp[2]*256 + buftmp[3];

		fseek(file, receivedData, SEEK_SET);

		for(i = 0; i < numPackets ;i++){
			fwrite(&buftmp[4+i], sizeof(unsigned char), 1, file);
		}

		receivedData += (numPackets);
		free(buftmp);

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
	    printf("Sent UA.\n");
    	toWrite[0] = FLAG; toWrite[1] = 0x03; toWrite[2] = state; toWrite[3] = state^0x03; toWrite[4] = FLAG;
	}
	else if (state == DISC) {
  		printf("Sent DISC.\n");
		toWrite[0] = FLAG; toWrite[1] = 0x01; toWrite[2] = state; toWrite[3] = state^0x01; toWrite[4] = FLAG;
	}
  	else if (state == RR) {
		printf("Sent RR.\n");
    	toWrite[0] = FLAG; toWrite[1] = 0x01; toWrite[2] = state^NR; toWrite[3] = state^0x01; toWrite[4] = FLAG;
  	}
	else if (state == REJ) {
		printf("Sent REJ.\n");
    	toWrite[0] = FLAG; toWrite[1] = 0x01; toWrite[2] = state^NR; toWrite[3] = state^0x01; toWrite[4] = FLAG;
  	}

	write(fd, toWrite, 5);
}

int llopen(int fd) {
  int res;
  printf("Entered llopen.\n");

  while (STOP==FALSE) {
	unsigned char* buf = (unsigned char*) malloc(MAX_SIZE);
    frread(fd,buf,MAX_SIZE);
	free(buf);
  }

  frwrite(fd, UA, 0x00);

  STOP = FALSE;
  return 0;
}

int llread(int fd) {
  	int res;
	printf("Entered llread.\n");
  	while (STOP==FALSE) {
    	unsigned char* buf = (unsigned char*) malloc(MAX_SIZE);
    	frread(fd,buf,MAX_SIZE);
		free(buf);
  	}

  	STOP = FALSE;
  	return 0;
}

int llclose(int fd) {
  	int res;
  	printf("Entered llclose.\n");
	frwrite(fd, DISC, 0x00);

  	while (STOP==FALSE) {
		unsigned char* buf = (unsigned char*) malloc(MAX_SIZE);
    	frread(fd,buf,MAX_SIZE);
		free(buf);
  	}
  	return 0;
}

unsigned char* destuff(unsigned char * buf, int * size) {
  int i, buf2Index = 0;
	int sizetmp = *size;
	int arraySize = sizetmp;
	unsigned char * buf2 = (unsigned char *)malloc(sizetmp);
	for (i = 0; i < sizetmp; i++, buf2Index++) {
	if (buf[i] == 0x7d && buf[i+1] == 0x5e) {
			buf2[buf2Index] = 0x7e;
			i++;
			arraySize--;
		}
    else if (buf[i] == 0x7d && buf[i+1] == 0x5d) {
			buf2[buf2Index] = 0x7d;
			arraySize--;
			i++;
		}
		else{
			buf2[buf2Index] = buf[i];
		}
  	}
  	*size = arraySize;
	buf2 = realloc(buf2,arraySize);
	return buf2;
}
