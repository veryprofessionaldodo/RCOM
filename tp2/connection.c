#include "connection.h"


//sends info to sockfd
int sendFTP(ftp* ftp, const char* str, size_t size) {
	int n;

	if ((n = write(ftp->sockfd, str, size)) <= 0) {
		printf("Error sending information.\n");
		return -1;
	}

	printf("\nInfo: %s\n", str);

	return 0;
}

//reads info from sockfd
int readFTP(ftp* ftp, char* str, size_t size) {
	FILE* file = fdopen(ftp->sockfd, "r");

	do {
		memset(str, 0, size);
		str = fgets(str, size, file);
		printf("%s", str);
	} while (!('1' <= str[0] && str[0] <= '5') || str[3] != ' ');

	return 0;
}

int connectSocket(const char* ip, int port) {
	int sockfd;
	struct sockaddr_in server_addr;

	// server address handling
	bzero((char*) &server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ip); /*32 bit Internet address network byte ordered*/
	server_addr.sin_port = htons(port); /*server TCP port must be network byte ordered */

	// open an TCP socket
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket()");
		return -1;
	}

	// connect to the server
	if (connect(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr))< 0) {
		perror("connect()");
		return -1;
	}

	return sockfd;
}

int connectFTP(ftp* ftp, const char* ip, int port) {
	int socketfd;
	char rd[512];

	if ((socketfd = connectSocket(ip, port)) < 0) {
		printf("ERROR: Cannot connect socket.\n");
		return 1;
	}

	ftp->datafd = 0;
	ftp->sockfd = socketfd;


	if (readFTP(ftp, rd, sizeof(rd))) {
		printf("ERROR: readFTP failure.\n");
		return 1;
	}

	return 0;
}



int CD_FTP(ftp* ftp, const char* path) {
	char cd[512];

	//'CWD' FTP command
	sprintf(cd, "CWD %s\r\n", path);

	if (sendFTP(ftp, cd, strlen(cd))) {
		printf("Error sending path to CWD command.\n");
		return -1;
	}

	if (readFTP(ftp, cd, sizeof(cd))) {
		printf("Error sending path to change directory.\n");
		return -1;
	}

	return 0;
}

int passiveFTP(ftp* ftp) {
	char pasv[512] = "PASV\r\n";

	if (sendFTP(ftp, pasv, strlen(pasv))) {
		printf("Error entering in passive mode.\n");
		return 1;
	}

	if (readFTP(ftp, pasv, sizeof(pasv))) {
		printf("No info received to enter in passive mode.\n");
		return -1;
	}

	// starting process information
	int h1, h2, h3, h4,p1, p2;

	if ((sscanf(pasv, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)", &h1,&h2, &h3, &h4, &p1, &p2)) < 0) {
		printf("Error processing information to calculating port.\n");
		return -1;
	}

	// cleaning buffer
	memset(pasv, 0, sizeof(pasv));

	// forming ip
	if ((sprintf(pasv, "%d.%d.%d.%d", h1, h2, h3, h4))< 0) {
		printf("Error forming ip address.\n");
		return -1;
	}

	// calculating new port
	int newPort = p1 * 256 + p2;

	printf("IP: %s\n", pasv);
	printf("PORT: %d\n", newPort);

	if ((ftp->datafd = connectSocket(pasv, newPort)) < 0) {
		printf("Error in file descriptor of data socket\n");
		return -1;
	}

	return 0;
}

int logInFTP(ftp* ftp, const char* user, const char* password) {
	char sd[512];

	// sends user info
	sprintf(sd, "USER %s\r\n", user);
	if (sendFTP(ftp, sd, strlen(sd))) {
		printf("Error sending username info.\n");
		return -1;
	}

	if (readFTP(ftp, sd, sizeof(sd))) {
		printf("Access denied (username read).\n");
		return -1;
	}

	// cleaning buffer
	memset(sd, 0, sizeof(sd));

	// sends password info
	sprintf(sd, "PASS %s\r\n", password);
	if (sendFTP(ftp, sd, strlen(sd))) {
		printf("Error sending pasword info.\n");
		return -1;
	}

	if (readFTP(ftp, sd, sizeof(sd))) {
		printf("Access denied (password read).\n");
		return -1;
	}

	return 0;
}

int RETR_FTP(ftp* ftp, const char* filename) {
	char retr[512];

	sprintf(retr, "RETR %s\r\n", filename);
	if (sendFTP(ftp, retr, strlen(retr))) {
		printf("Error sending filename.\n");
		return -1;
	}

	if (readFTP(ftp, retr, sizeof(retr))) {
		printf("Error no information received.\n");
		return -1;
	}

	return 0;
}

int transferFTP(ftp* ftp, const char* filename) {
	FILE* file = fopen(filename, "w");
	int n;

	if (!file) {
		printf("Error opening download file.\n");
		return -1;
	}

	char buf[1024];
	while ((n = read(ftp->datafd, buf, sizeof(buf)))) {
		if (n < 0) {
			printf("Error receiving data.\n");
			return -1;
		}

		if ((n = fwrite(buf, n, 1, file)) < 0) {
			printf("Error riting into file.\n");
			return -1;
		}
	}

	fclose(file);
	close(ftp->datafd);

	return 0;
}

int disconnectFTP(ftp* ftp) {
	char quit[512];

	if (readFTP(ftp, quit, sizeof(quit))) {
		printf("Error disconnecting.\n");
		return -1;
	}

	//"QUIT" comand
	sprintf(quit, "QUIT\r\n");

	if (sendFTP(ftp, quit, strlen(quit))) {
		printf("Error sending quit command\n");
		return -1;
	}

	if (ftp->sockfd)
		close(ftp->sockfd);

	return 0;
}


