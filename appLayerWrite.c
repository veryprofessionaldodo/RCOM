/*Non-Canonical Input Processing*/
#include "dataLayerWrite.h"

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

struct linkLayer ll;


unsigned char* buildControlPacket(char state,char * filename,int *sizeFile){

  unsigned char filesize[4];
  int sizetmp = *sizeFile;

  filesize[0] = (sizetmp >> 24) & 0xFF;
  filesize[1] = (sizetmp >> 16) & 0xFF;
  filesize[2] = (sizetmp >> 8) & 0xFF;
  filesize[3] = sizetmp & 0xFF;

  int size = 5 + 4 + strlen(filename);

  unsigned char* packet = malloc(size);

  unsigned int i=0,pos=3;

  packet[0] = state;
  packet[1] = 0x01; //filename
  packet[2] = strlen(filename);

  for(i = 0; i < strlen(filename);i++){
    packet[pos++] = filename[i];
  }

  packet[pos++]=0x00; //filesize
  packet[pos++]= sizeof(filesize);

  for(i = 0; i < 4; i++){
    packet[pos++] = filesize[i];
  }
  sizetmp = size;
  *sizeFile = sizetmp;

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

    /*ll.port = argv[1];
	ll.sequenceNumber = 0;
	ll.timeout = 3;*/

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

        unsigned char* CTRL_START = buildControlPacket(0x02,argv[2],&size);

        /*int i ;
          for(i = 0; i < size; i++){
            printf("packets[%d] =  %x \n",i,CTRL_START[i] );
          }*/

        /*int i = 0;
        while(!feof(file)){
        	unsigned char* buf = (unsigned char*)malloc (sizeof(unsigned char)*10);
        	fread(buf,sizeof(unsigned char*),sizeof(unsigned char*)*10,file);
        	free(buf);
        }*/



        if(llwrite(fd,CTRL_START,size) < 0)
          printf("ERROR in llwrite start! \n");

         printf("read start : pass to end\n");
        //send ctrl end
        int size_ctrl_end = st.st_size;
        unsigned char* CTRL_END = buildControlPacket(0x03,argv[2],&size_ctrl_end);

          if(llwrite(fd,CTRL_END,size_ctrl_end) < 0)
          printf("ERROR in llwrite end! \n");

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
