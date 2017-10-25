/*Non-Canonical Input Processing*/
#include "dataLayerWrite.h"

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

int nSequence = 0;
int transmittedData = 0;


unsigned char* buildControlPacket(char state,char * filename,int sizeFile){
  unsigned char filesize[4];
  int sizetmp = sizeFile;

  filesize[0] = (sizetmp >> 24) & 0xFF;
  filesize[1] = (sizetmp >> 16) & 0xFF;
  filesize[2] = (sizetmp >> 8) & 0xFF;
  filesize[3] = sizetmp & 0xFF;

  int size = 5 + 4 + strlen(filename);

  unsigned char* packet = malloc(size);

  unsigned int i=0;

  packet[0] = state;
  packet[1] = 0x01; //filename
  packet[2] = strlen(filename);

  for(i = 0; i < strlen(filename);i++){
    packet[3+i] = filename[i];
  }

  int nextPos = 3 + strlen(filename);

  packet[nextPos]=0x00; //filesize
  packet[nextPos+1]= sizeof(filesize);

  for(i = 0; i < 4; i++){
    packet[nextPos +2 +i] = filesize[i];
  }

  return packet;
}

unsigned char* buildDataPacket(char * buf,int sizeFile){
 unsigned char filesize[4];
  int sizetmp = sizeFile;

  int L2 = sizetmp / 256;
	int L1 = sizetmp % 256;

  int size = sizetmp + 4;

  unsigned char* packet = malloc(size);

  unsigned int i=0,pos=4;

  packet[0] = 0x01;
  packet[1] = nSequence%255;
  packet[2] = L2;
	packet[3] = L1;

  for(i = 4; i < size; i++){
    packet[i] = buf[i-4];
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

  newtio.c_cc[VTIME] = 30;   /* inter-character timer unused */
  newtio.c_cc[VMIN]  = 0;   /* blocking read until 5 chars received */

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
        int size = 5+ 4+ strlen(argv[2]);
        printf("size %d\n", size);

        unsigned char* CTRL_START = buildControlPacket(0x02,argv[2],st.st_size); //control start packet created

				if(llwrite(fd,CTRL_START,size) < 0)   //send control start packet
          printf("ERROR in llwrite start! \n");

        int i = 0;
        FILE* file2;
        file2 = fopen("yolo2.gif", "w");
          if (file2 == NULL) {
          perror ("Error opening file");
        printf("ERROR in llwrite! \n");
        }
				unsigned char* buf = (unsigned char*) malloc (st.st_size);
        printf("st_size = %d\n",st.st_size );

				fread(buf,sizeof(unsigned char),st.st_size,file);    //read from file


        /*for(i = 0; i < st.st_size ; i++) {
           printf(" [%d] = %x ", i, buf[i]);
        }*/



	      int sizebuf;
        while(transmittedData < st.st_size){
          //printf("buf = %s\n",buf );
					     if ((st.st_size - transmittedData) > MAX_SIZE)
                  sizebuf = MAX_SIZE;
					      else
						      sizebuf = st.st_size - transmittedData;

          unsigned char* buftmp = (unsigned char*)malloc (sizebuf);

          memcpy(buftmp,buf + transmittedData, sizebuf);
  //        printf("strlen %d\n",strlen(buftmp) );

          fseek(file2,transmittedData,SEEK_SET);

          fwrite(buftmp, sizeof(unsigned char), sizebuf, file2);
        //  printf("buf[8] %x buf[n] %x\n",buftmp[0],buftmp[sizebuf]);

					unsigned char * CTRL_DATA =buildDataPacket(buftmp,sizebuf);      //create data packet

          free(buftmp);

          if(llwrite(fd,CTRL_DATA,sizebuf+4) < 0)  //send control data packet
            printf("ERROR in llwrite data! \n");


          if (transmittedData + sizebuf > st.st_size)  {
              transmittedData += st.st_size;
            }
        else
             transmittedData += sizebuf;




          printf("transmitted data %d st.st_size %d sizebuf %d\n", transmittedData, st.st_size, sizebuf);
        }
        fclose (file2);
        free(buf);

        printf("finished writing!!!!\n");
        //send ctrl end
        unsigned char* CTRL_END = buildControlPacket(0x03,argv[2],st.st_size); //control end packet created

        if(llwrite(fd,CTRL_END,size) < 0)  //send control end packet
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
