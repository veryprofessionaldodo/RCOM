/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
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
#define CONTROL_PACKET_START 2
#define CONTROL_PACKET_END 3

FILE * file;
volatile int STOP=FALSE;

char filename;
int filesize;

int main(int argc, char** argv)
{
    int fd,c, res;
    struct termios oldtio,newtio;
    char buf[255];

    if ( (argc < 2) || 
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) && 
  	      (strcmp("/dev/ttyS1", argv[1])!=0) )) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(1);
    }


  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */
  
    
    fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (fd <0) {perror(argv[1]); exit(-1); }

    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
      perror("tcgetattr");
      exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 5;   /* blocking read until 5 chars received */



  /* 
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a 
    leitura do(s) próximo(s) caracter(es)
  */



    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    printf("New termios structure set\n");



    while (STOP==FALSE) {
         char buf[255];   /* loop for input */
	     res = frread(fd,buf,sizeof(buf));   /* returns after 5 chars have been input */
	}
  /*
    O ciclo WHILE deve ser alterado de modo a respeitar o indicado no gui�o
  */


    tcsetattr(fd,TCSANOW,&oldtio);
    close(fd);
    return 0;
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
			return n;
		}
	}
}

void processframe(int fd, char* buf, int n) {

  char r = 0x00;

  if (buf[2] == NS0)
    r = NR1;
  else
    r = NR0;

  if (hasErrors(buf)) {
      frwrite(fd, REJ, r);
      return;
  }

	if (n == 5) {
		// Check if SET
		if (buf[0] == FLAG && buf[1] == 0x03 && buf[2] == SET
			&& buf[3] == SET^0x03 && buf[4] == FLAG) {
	        printf("recebi um set\n");
	        frwrite(fd, UA, n, 0x00);
		}
		// Check if DISC
		else if (buf[0] = 	= FLAG && buf[1] == 0x03 && buf[2] == DISC
			&& buf[3] == DISC^0x03 && buf[4] == FLAG) {
           printf("recebi um disc\n");
			frwrite(fd, DISC, n, 0x00);
		}

		// Check if UA
		else if (buf[0] == FLAG && buf[1] == 0x01 && buf[2] == UA
			&& buf[3] == DISC^0x01 && buf[4] == FLAG) {
				STOP = TRUE;
		}
	}

	else {
		processInformationFrame(fd, buf);
	}

}

void processInformationFrame(int fd, char* buf) {
	// I Need to see if it's a control or data packet

  char r;
  if (buf[2] == NS0)
    r = NR1;
  else
    r = NR0;

	// Control Start
	if (buf[4] == CONTROL_PACKET_START) { // We chose the first T to be filename, and the second to be size
		int nameSize = (int)strtol(buf[6], NULL, 0);

		memcpy(filename, buf + 7, nameSize * sizeof(char)); 
		int nextPos = nameSize * sizeof(char);		

		int fileInformationSize = (int)strtol(buf[nextPos+1], NULL, 0);
		char fileBuffer[fileInformationSize];

		memcpy(fileBuffer, buf+ nextPos+2, fileInformationSize * sizeof(char)); 
		(int)strtol(buf[6], NULL, 0);
		
			
	}
	// Control End
	else if (buf[4] == CONTROL_PACKET_END) {

	}
	  // Data Packet
  else {
    	// Write on the file data size
    	frwrite(fd, RR, r);
  }
}

int hasErrors(char * buf) {

  // See if frame has any errors
  return FALSE;

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
