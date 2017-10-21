/*Non-Canonical Input Processing*/
#include "dataLayerWrite.h"

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1


unsigned char* buildStartPacket(char * filename,int sizeFile){

  unsigned char filesize[4];

  filesize[0] = (sizeFile >> 24) & 0xFF;
  filesize[1] = (sizeFile >> 16) & 0xFF;
  filesize[2] = (sizeFile >> 8) & 0xFF;
  filesize[3] = sizeFile & 0xFF;

  int size = 5 + 4 + strlen(filename);

  unsigned char* packet = malloc(size);

  unsigned int i=0,pos=3;

  packet[0] = 0x02;
  packet[1] = 0x01; //filename
  packet[2] = strlen(filename);

  for(i = 0; i < strlen(filename);i++){
    packet[pos++] = filename[i];
  }

  packet[pos++]=0x00; //filesize
  packet[pos++]= sizeof(filesize);
  printf("filesize %d", sizeof(filesize));

  for(i = 0; i < 4; i++){
    packet[pos++] = filesize[i];
  }

  return packet;
}

int main(int argc, char** argv){
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
  FILE *file;

    if(llopen(fd) < 0)
      printf("ERROR in llopen! \n");
      else{

        file = fopen(argv[2], "r");
          if (file == NULL) {
          perror ("Error opening file");
        printf("ERROR in llwrite! \n");
         fclose (file);
        }

        //size of file
        struct stat st;
        stat(argv[2], &st);
        int size = st.st_size;

        unsigned char* CTRL_START = buildStartPacket(argv[2],size);

        int i ;
          for(i = 0; i < 17; i++){
            printf("packets[%d] =  %x \n",i,CTRL_START[i] );
          }
        /*int i = 0;
        while(!feof(file)){
        	unsigned char* buf = (unsigned char*)malloc (sizeof(unsigned char)*10);
        	fread(buf,sizeof(unsigned char*),sizeof(unsigned char*)*10,file);
        	stuff(buf);
        	free(buf);
        }*/

        if(llwrite(fd,CTRL_START,17) < 0)
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
