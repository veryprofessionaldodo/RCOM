/*Non-Canonical Input Processing*/
#ifndef dataLayerWrite
#define dataLayerWrite

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
#define FLAG 0x7e
#define SET 0x03
#define UA 0x07
#define DISC 0x0b
#define ESC 0x5e
#define MAX_SIZE 1024
#define RR 0x05
#define REJ 0x01
#define NS0 0x40
#define NS1 0x00
#define NR0 0x80
#define NR1 0x00

void processframe(int fd,unsigned  char* buf, unsigned int n);

int frread(int fd, unsigned char * buf, int maxlen);

void frwrite(int fd, char state, int n);

void incCounter();

int llopen(int fd);

int llclose(int fd);

int llwrite(int fd, unsigned char* buf,int size);

unsigned char * stuff(unsigned char *buf, unsigned int* size);

#endif
