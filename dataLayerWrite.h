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
#define DISC 0x0B
#define ESC 0x5e
#define MAX_SIZE 1024
#define RR 0x05
#define REJ 0x01

// States
#define CONNECTING 0
#define WAITING_FOR_RESPONSE 1
#define SENDING 2
#define DISCONNECTING 3
#define RETRYING_LAST_OPERATION 4

struct linkLayer {
char port[20];/*Dispositivo /dev/ttySx, x = 0, 1*/
int baudRate;/*Velocidade de transmissão*/
unsigned int sequenceNumber;   /*Número de sequência da trama: 0, 1*/
unsigned int timeout;/*Valor do temporizador: 1 s*/
unsigned int numTransmissions; /*Número de tentativas em caso de falha*/
char frame[MAX_SIZE];/*Trama*/
};

void processframe(int fd, char* buf, int n);

int frread(int fd, unsigned char * buf, int maxlen);

void frwrite(int fd, char state, int n);

void incCounter();

int llopen(int fd);

int llclose(int fd);

int llwrite(int fd, unsigned char* buf,int size);

void stuff(unsigned char *buf, unsigned int* size);

#endif
