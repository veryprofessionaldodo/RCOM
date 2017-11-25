/*Non-Canonical Input Processing*/

#include "dataLayerRead.h"

volatile int STOP=FALSE;

FILE* file;
unsigned int filesize;
unsigned int receivedData = 0;
unsigned char previousSend = 0xFF; // Different than every possibility
unsigned char previousRequest = NR1;

unsigned char * previousReceivedFrame;

int frread(int fd, unsigned char * buf, int maxlen) {
	int n=0;
	int ch;

	while(1) {
 		if((ch= read(fd, buf + n, 1)) <= 0) { //reads form serial port
			return ch; // ERROR
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
			return 0;
		}
	}
	return 0;
}

//function made to check if there are any errors in the frames
int hasErrors(unsigned char * buf, int size) {

#ifdef TEST
  int r = rand(100);
  if (r < PROBABILITY_OF_ERROR)
  	return TRUE;
#endif

  if (size == 5) {
		if (buf[2] == SET || buf[2] == DISC) {
				if ((buf[2]^0x03) != buf[3]) { //checks if there is a error in set or disc frame
					printf("ERROR ON SET or DISC!\n");
					return TRUE;
				}
				else
					return FALSE;
		}
		else {
			if (buf[2] == UA) {
				if ((buf[2]^0x01) != buf[3]) {//checks if there is a error in ua frame
					printf("ERROR ON UA!\n");
					return TRUE;
				}
			}
			else
				return FALSE;
		}

	}

	else { //checks if there is a error in Information frame
			unsigned char BCC2 = 0x00;

			if ((buf[2]^0x03) != buf[3]) {
				printf("ERROR ON Information Header!\n");
				return TRUE;
			}
			else { // Check errors in message
					int x;
					for (x = 4; x < size-2;x++) {
						BCC2 = (BCC2^buf[x]);
					}
					if (BCC2 != buf[size-2]){
						printf("ERROR ON Information Frame!\n");
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

	if (n == 5) {
	  	if (hasErrors(buf,5)) { //checks if there are any errors in the frame
	      frwrite(fd, REJ, 0x00);
	      return -1;
	  	}
		// Check if SET
		if (buf[0] == FLAG && buf[1] == ADDRESS2 && buf[2] == SET
			&& buf[3] == SET^ADDRESS2 && buf[4] == FLAG) {
	        printf("Received SET.\n");
	        STOP = TRUE;//closes llopen
			return 0;
		}// Check if DISC
		else if (buf[0] == FLAG && buf[1] == ADDRESS2 && buf[2] == DISC
			&& buf[3] == DISC^ADDRESS2 && buf[4] == FLAG) {
           printf("Received DISC.\n");
           STOP = TRUE; //closes llread
           return 0;
		}// Check if UA
		else if (buf[0] == FLAG && buf[1] == ADDRESS1 && buf[2] == UA
			&& buf[3] == DISC^ADDRESS1 && buf[4] == FLAG) {
			 printf("Received UA.\n");
			 STOP = TRUE; //closes llclose
			 return 0;
		}
	}
	else {
		processInformationFrame(fd, buf, n); //processes information frames
	}

	return 0;

}

//function designed to process information frames
void processInformationFrame(int fd, unsigned char* buf, int n) {

	int i = n;
	buf = destuff(buf, &i);

//checks if there are any errors in information frame
  if (hasErrors(buf,i)) {
      frwrite(fd, REJ, previousRequest);
      return;
 }

	char request;

  	if (buf[2] == NS0)
    	request = NR1;
  	else
    	request = NR0;

	if (previousSend == buf[2]) { //checks if its a duplicate frame
		printf("Duplicate, not writing to file.\n");
		frwrite(fd, RR, previousRequest);
		return;
	}

	else {
		previousSend = buf[2];
		previousRequest = request;
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
		unsigned char * filesizeChar = (unsigned char *) malloc(fileInformationSize * sizeof(unsigned char*));

		for (x = 0; x < 4; x++) {
			filesizeChar[x] = buf[x+nextPos+9];
		}

		filesize = *(int *)filesizeChar;

	  	file = fopen(filename, "wb");
		if (file == NULL)
			printf("Error opening file!\n");

		frwrite(fd, RR, request); //sends RR frame
	}

	// Control End
	else if (buf[4] == CONTROL_PACKET_END) {
		printf("Reading Control Packet End.\n");
		fclose(file);
		frwrite(fd, RR, request);
	}

  	// Data Packet
  	else if (buf[4] == 0x01) {
		int i;
		unsigned char * buftmp = (unsigned char*)malloc(n-5);

		memcpy(buftmp, buf + 4, n-5);

		int sizeOfBuftmp = n-5;
		int numPackets = buftmp[2]*256 + buftmp[3];

		fseek(file, receivedData, SEEK_SET);

		for(i = 0; i < numPackets ;i++){
			fwrite(&buftmp[4+i], sizeof(unsigned char), 1, file); //writes to the file
		}

		receivedData += (numPackets);
		free(buftmp);

		frwrite(fd, RR, request);
  	}

	else { // Not a valid packet
		frwrite(fd, REJ, request);
	}
}

void frwrite(int fd, char state, char NR) {
	unsigned char toWrite[5];

		// Isn't information frame
	if (state == UA) {
	    printf("Sent UA.\n");
    	toWrite[0] = FLAG; toWrite[1] = ADDRESS2; toWrite[2] = state; toWrite[3] = state^ADDRESS2; toWrite[4] = FLAG;
	}
	else if (state == DISC) {
  		printf("Sent DISC.\n");
		toWrite[0] = FLAG; toWrite[1] = ADDRESS1; toWrite[2] = state; toWrite[3] = state^ADDRESS1; toWrite[4] = FLAG;
	}
  	else if (state == RR) {
		printf("Sent RR.\n");
    	toWrite[0] = FLAG; toWrite[1] = ADDRESS1; toWrite[2] = state^NR; toWrite[3] = state^ADDRESS1; toWrite[4] = FLAG;
  	}
	else if (state == REJ) {
		printf("Sent REJ.\n");
    	toWrite[0] = FLAG; toWrite[1] = ADDRESS1; toWrite[2] = state^NR; toWrite[3] = state^ADDRESS1; toWrite[4] = FLAG;
  	}

	write(fd, toWrite, 5);
}

int llopen(int fd) {
  printf("Entered llopen.\n");

  while (STOP==FALSE) {
	unsigned char* buf = (unsigned char*) malloc(MAX_SIZE);
    frread(fd,buf,MAX_SIZE);
	free(buf);
  }

  frwrite(fd, UA, 0x00); //sends UA frame
  STOP = FALSE;
  return 0;
}

int llread(int fd) {
	printf("Entered llread.\n");

  	while (STOP==FALSE) {
    	unsigned char* buf = (unsigned char*) malloc(MAX_SIZE + 7);
    	frread(fd,buf,MAX_SIZE);
		free(buf);
  	}

  	STOP = FALSE;
  	return 0;
}

int llclose(int fd) {
  	printf("Entered llclose.\n");

	frwrite(fd, DISC, 0x00); //sends DISC frame

  	while (STOP==FALSE) {
		unsigned char* buf = (unsigned char*) malloc(MAX_SIZE + 7);
    	frread(fd,buf,MAX_SIZE);
		free(buf);
  	}
  	return 0;
}

unsigned char* destuff(unsigned char * buf, int * size) {
  int i, buf2Index = 0;
  int sizetmp = *size;
  int arraySize = sizetmp;

  unsigned char * buf2 = (unsigned char *)malloc(sizetmp); //alocates memory to the new buffer

  for (i = 0; i < sizetmp; i++, buf2Index++) {
  	if (buf[i] == 0x7d && buf[i+1] == 0x5e) {  //checks if it is an escape flag
		  buf2[buf2Index] = 0x7e;
		  i++;
		  arraySize--;
	  }
  	else if (buf[i] == 0x7d && buf[i+1] == 0x5d) { //checks if it is an escape flag
		  buf2[buf2Index] = 0x7d;
		  arraySize--;
		  i++;
	  }
	else{
		  buf2[buf2Index] = buf[i];
	  }
  }
  *size = arraySize;
  buf2 = realloc(buf2,arraySize); //reallocates memory to the returned buffer
  return buf2;
}
