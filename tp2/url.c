#include "url.h"

int parseURL(url* url, const char* str) {
	char* urltmp, *strtmp;
	int passmode;

	strtmp = (char*) malloc(strlen(str));
	urltmp = (char*) malloc(strlen(str));

	memcpy(urltmp, str, strlen(str));

	url->user = malloc(256);
	url->password = malloc(256);
	url->filename = malloc(256);
	url->host = malloc(256);
	url->path = malloc(256);

	memset(url->user, 0, 256);
	memset(url->password, 0, 256);
	memset(url->filename, 0, 256);
	memset(url->path, 0, 256);
	memset(url->host, 0, 256);
	url->port = FTP_PORT;


	//checks if usage normal or anonymous
	if (urltmp[6] == '[')
		passmode = 0;
	else 
		passmode = 1;
	

	// removing ftp:// from str
	strcpy(urltmp, urltmp + 6);

	//if not anonymous
	if (!passmode) {

		//removing '[' from str
		strcpy(urltmp, urltmp + 1);

		// parsing username
		char * username = parseStrToChar(urltmp, ':');
		strcpy(strtmp, username);
		memcpy(url->user, strtmp, strlen(strtmp));

		//parsing password
		char * password = parseStrToChar(urltmp, '@');
		strcpy(strtmp, password);
		memcpy(url->password, strtmp, strlen(strtmp));

		//removing ']' from str
		strcpy(urltmp, urltmp + 1);

		free(username);
		free(password);
	}
	else{  //anonymous mode

	//user "anonymous"
	url->user = "anonymous";
	url->password = "anonymous";
	}

	//parsing host
	char * host = parseStrToChar(urltmp, '/');
	strcpy(strtmp, host);
	memcpy(url->host, strtmp, strlen(strtmp));

	//saving url path
	char* path = (char*) malloc(strlen(urltmp));
	int startPath = 1;

	//while exists '/'
	while (strchr(urltmp, '/')) {
		strtmp = parseStrToChar(urltmp, '/');

		if (startPath) {
			startPath = 0;
			strcpy(path, strtmp);
		} else {
			strcat(path, strtmp);
		}

		strcat(path, "/");
	}

	//saving url path
	strcpy(url->path, path);

	//saving filename
	strcpy(url->filename, urltmp);

	free(host);
	free(path);
	free(urltmp);
	free(strtmp);

	return 0;
}

int getIpByHost(url* url) {
	struct hostent* h;

	if ((h = gethostbyname(url->host)) == NULL) {
		herror("gethostbyname");
		return 1;
	}

	//allocates memory to ip string
	url->ip = malloc(256);

	char* ip = inet_ntoa(*((struct in_addr *) h->h_addr));
	strcpy(url->ip, ip);

	return 0;
}

char* parseStrToChar(char* strtmp, char c) {

	char* tempStr = (char*) malloc(strlen(strtmp));

	// length of tempStr
	int index = strlen(strtmp) - strlen(strcpy(tempStr, strchr(strtmp, c)));

	tempStr[index] = '\0'; // end of string
	strncpy(tempStr, strtmp, index);
	strcpy(strtmp, strtmp + strlen(tempStr) + 1);

	return tempStr;
}
