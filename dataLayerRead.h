/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

int main(int argc, char** argv);

int frread(int fd, unsigned char * buf2, int maxlen);

int processframe(int fd, char* buf, int n);

void processInformationFrame(int fd, char* buf);

int hasErrors(char * buf);

void frwrite(int fd, char state, char NR);
