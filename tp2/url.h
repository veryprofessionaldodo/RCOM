#include <netdb.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

#define FTP_PORT 21

typedef struct URL {
	char* user; // user
	char* password; // password
	char* ip; // ip string
	char* host; // host string
	char* filename; // filename
	char* path; // path
	int port; // port integer
} url;

int parseURL(url* url, const char* str); // Parse "str" to url struct
int getIpByHost(url* url); // gets ip by host name
char* parseStrToChar(char* str, char chr); //parse string to char
