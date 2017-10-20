/*Non-Canonical Input Processing*/
#include "dataLayerWrite.h"

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1


int main(int argc, char** argv)
{
  int fd,c, res;
  struct termios oldtio,newtio;
  char buf[255];
  int i, sum = 0, speed = 0;

  if ( (argc < 3) ||
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

    if(llopen(fd) < 0)
      printf("ERROR in llopen! \n");
      else{
        FILE *file;
        file = fopen(argv[2], "r");
          if (file == NULL) {
          perror ("Error opening file");
        printf("ERROR in llwrite! \n");
         fclose (file);
        }

        //size of file
        struct stat st;
        stat(argv[2], &st);
        unsigned long int size = st.st_size;
        unsigned char filesize[4];

filesize[0] = (size >> 24) & 0xFF;
filesize[1] = (size >> 16) & 0xFF;
filesize[2] = (size >> 8) & 0xFF;
filesize[3] = size & 0xFF;

        char* buf = buildStartPacket(argv[2],filesize);

        /*int i = 0;
        while(!feof(file)){
        	unsigned char* buf = (unsigned char*)malloc (sizeof(unsigned char)*10);
        	fread(buf,sizeof(unsigned char*),sizeof(unsigned char*)*10,file);
        	stuff(buf);
        	free(buf);
        }*/

        if(llwrite(fd,buf) < 0)
          printf("ERROR in llwrite! \n");
        else{
         if(llclose(fd) < 0)
          printf("ERROR in llclose! \n");
         else
          return 0;
         }
      }

    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    close(fd);
    fclose(file);
    return 0;
}

char* buildStartPacket(char * filename,char * filesize){
  char c[255];
  c[0] = 0x02;
  c[1] = 0x01; //filename
  c[2] = sizeof(filename);

  c[7] = 0x00;  //filesize
  c[8] = sizeof(filesize);
  c[3] = filesize[0];
  c[4] = filesize[1];
  c[5] = filesize[2];
  c[6] = filesize[3];




}
