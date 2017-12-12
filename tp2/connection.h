#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <regex.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

typedef struct FTP
{
    int sockfd; // file descriptor to socket control
    int datafd; // file descriptor to transfer data
} ftp;


int sendFTP(ftp* ftp, const char* str, size_t size);

int readFTP(ftp* ftp, char* str, size_t size);

int connectFTP(ftp* ftp, const char* ip, int port);

int connectSocket(const char* ip, int port);

int logInFTP(ftp* ftp, const char* user, const char* password);

int CD_FTP(ftp* ftp, const char* path);

int RETR_FTP(ftp* ftp, const char* filename);

int transferFTP(ftp* ftp, const char* filename);

int passiveFTP(ftp* ftp);

int disconnectFTP(ftp* ftp);



