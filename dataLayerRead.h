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


void frwrite(int fd, char state, char NR);

int processframe(int fd, char* buf, int n);

void processInformationFrame(int fd, unsigned char* buf, int n);

int frread(int fd, unsigned char * buf2, int maxlen);

int llopen(int fd);

int llread(int fd);

int llclose(int fd);

int hasErrors(char * buf);

unsigned char* destuff(unsigned char * buf);
