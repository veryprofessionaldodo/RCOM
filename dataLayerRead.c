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

int hasErrors(unsigned char * buf) {
  // See if frame has any errors
  return FALSE;
}

int processframe(int fd, unsigned char* buf, int n) {
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
	//printf("buf merda %x e final %x\n", buf[0], buf[n-1]);

	char r;
  if (buf[2] == NS0)
    r = NR1;
  else
    r = NR0;

		int i;



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

		unsigned char *filesizeChar = (unsigned char *) malloc(fileInformationSize * sizeof(unsigned char*));

		for (x = 0; x < 4; x++) {
			filesizeChar[x] = buf[x+nextPos+9];
		}

		filesize = *(int *)filesizeChar;

		printf("filesize %d\n", filesize);

	  file = fopen(filename, "wb");
		if (file == NULL)
			printf("Error opening file!\n");

		printf("pressupostamente acabei de ler um start\n");
		frwrite(fd, RR, r);
	}

	// Control End
	else if (buf[4] == CONTROL_PACKET_END) {
		printf("pressupostamente estou a ler um end\n");
		fclose(file);
		frwrite(fd, RR, r);
	}


	  // Data Packet
  else if (buf[4] == 0x01) {
		//printf("pressupostamente estou a ler dados mesmo\n");


		int i;

		//printf("tamanho %d  n %d\n", n-10, n);

		unsigned char * buftmp = (unsigned char*)malloc(n-5);

		memcpy(buftmp, buf + 4, n-5);

		/*for (i = 0; i < n; i++) {
			printf("buf [%d] = %x", i, buf[i]);
		}*/

    buftmp = destuff(buftmp,n-5);

		int numPackets = buftmp[2]*256 + buftmp[3];

		//printf("recebido %d  n %d numpackets %d\n", receivedData, n,numPackets);

		/*for (i = 0 ; i < n-6; i++) {
			printf("buf [%d] = %x ",i, buftmp[i]);
		}*/

		fseek(file, receivedData, SEEK_SET);

		for(i = 0; i < numPackets ;i++){
			fwrite(&buftmp[4+i], sizeof(unsigned char), 1, file);
		//printf("buf [%d] = %x ",i, buftmp[i]);
		}

		receivedData += (numPackets);
		//printf("escrevi no ficheiro %d packets\n", receivedData);
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
	    printf("mandei um UA para o fd %d\n", fd);
    	toWrite[0] = FLAG; toWrite[1] = 0x03; toWrite[2] = state; toWrite[3] = state^0x03; toWrite[4] = FLAG;
	}
	else if (state == DISC) {
  	printf("mandei um DISC para o fd %d\n", fd);
		toWrite[0] = FLAG; toWrite[1] = 0x01; toWrite[2] = state; toWrite[3] = state^0x01; toWrite[4] = FLAG;
	}
  else if (state == RR || state == REJ) {
		printf("mandei um %x para o fd %d\n", state, fd);
    toWrite[0] = FLAG; toWrite[1] = 0x01; toWrite[2] = state^NR; toWrite[3] = state^0x01; toWrite[4] = FLAG;
  }

	printf("buf[2] %x \n", toWrite[2]);

	write(fd, toWrite, 5);
}

int llopen(int fd) {
  int res;

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
  printf("entrei no llclose\n");
	frwrite(fd, DISC, 0x00);

  while (STOP==FALSE) {
		unsigned char* buf = (unsigned char*) malloc(MAX_SIZE);
		printf("estou a ler\n");
    frread(fd,buf,MAX_SIZE);
		free(buf);
  }
  return 0;
}

unsigned char* destuff(unsigned char * buf, int size) {
  int i, buf2Index = 0;
	int sizetmp = size;
	unsigned char * buf2 = (unsigned char *)malloc(size);
	for (i = 0; i < size; i++, buf2Index++) {
		if(buf[i] == 0x7d) {
			printf("potencialmente destuff, seguinte é %x \n", buf[i+1]);
		}
    if (buf[i] == 0x7d && buf[i+1] == 0x5e) {
			printf("entrei no destuff\n");
			buf2[buf2Index] = 0x7e;
			i++;
			sizetmp--;
		}
    else if (buf[i] == 0x7d && buf[i+1] == 0x5d) {
			printf("entrei no destuff2\n");
			buf2[buf2Index] = 0x7d;
			sizetmp--;
			i++;
		}
		else{
			buf2[buf2Index] = buf[i];
		}
  }
	buf2 = realloc(buf2,sizetmp);
	return buf2;
}
