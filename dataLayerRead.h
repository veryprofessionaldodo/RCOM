/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
#define MAX_SIZE 1024
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

void frwrite(int fd, char state, char NR);

int processframe(int fd, unsigned char* buf, int n);

void processInformationFrame(int fd, unsigned char* buf, int n);

int frread(int fd, unsigned char * buf2, int maxlen);

int llopen(int fd);

int llread(int fd);

int llclose(int fd);

int hasErrors(unsigned char * buf, int size);

unsigned char* destuff(unsigned char * buf, int * size);
