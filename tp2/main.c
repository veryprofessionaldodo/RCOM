#include <stdio.h>
#include "url.h"
#include "connection.h"


int main(int argc, char** argv) {
	
	if (argc != 2) {
		printf("WARNING: Wrong number of arguments.\n");
		printf("Usage : %s ftp://[<user>:<password>@]<host>/<url-path>\n   OR    \n",argv[0]);
		printf("Usage Anonymous: %s ftp://<host>/<url-path>\n\n", argv[0]);
		return -1;
	}

	//start processing url
	url url;

	// parsing url
	if (parseURL(&url, argv[1]))
		return -1;

	// get ip by host
	if (getIpByHost(&url)) {
		printf("Error finding ip.\n");
		return -1;
	}

	printf("\nHOST: %s\n", url.host);
	printf("IP: %s\n\n", url.ip);
	

	//start FTP process

	ftp ftp;
	connectFTP(&ftp, url.ip, url.port);

	// Sending credentials to server
	if (logInFTP(&ftp, url.user, url.password)) {
		printf("Error in login user %s\n", url.user);
		return -1;
	}

	// Changes directory
	if (CD_FTP(&ftp, url.path)) {
		printf("Cannot change directory \n");
		return -1;
	}

	// Passive mode
	if (passiveFTP(&ftp)) {
		printf("Error in passive mode\n");
		return -1;
	}

	// says to server to start transfering file
	if(RETR_FTP(&ftp, url.filename)){
		printf("Error in RETR\n");
		return -1;
	}

	// donwloads file
	if(transferFTP(&ftp, url.filename)){
		printf("Error transfering the file\n");
		return -1;
	}

	// disconnects from the server
	if(disconnectFTP(&ftp)){
		printf("Error desconnecting function\n");
		return -1;
	}

	return 0;
}