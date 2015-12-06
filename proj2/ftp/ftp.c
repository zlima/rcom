/*      (C)2000 FEUP  */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <strings.h>


#define SERVER_PORT 6000
#define SERVER_ADDR "192.168.28.96"

#define TIMEOUT 5

void parseUrl(char* user, char* pass, char* host, char* path, char* filename){

	if(sscanf(buf, "ftp://%[^:]:%[^@]@%[^/]/%s", user, pass, host, path) < 4)
	{
		if(sscanf(buf, "ftp://%[^/]/%s", host, path) < 2) //no user
		{
			printf("Invalid URL\n");
			exit(1);
		}
		else //success
		{
			strcpy(user, "Anonymous");
		}

	}

	char * pch;
  	pch=strrchr(path,'/'); //search for the last occurence of /

  	if(pch != NULL)
		strcpy(filename, path+(pch-path+1)); //only keep filename
	else
		strcpy(filename, path); //file is in the root directory
	if(filename[0]=='\0')//empty filename (invalid)
	{
		printf("Invalid URL\n");
		exit(1);
	}	
}


void readMessage(char* buf, int sockfd, int ftpcode){

	int receivedBytes, firstChunk=1, messageCode;
	receivedBytes = readBuf(buf, sockfd);
	while(buf[receivedBytes-3]=='-' && receivedBytes > 0){
		buf[receivedBytes] = 0;
		printf("%s", buf);
		if(firstChunk){
			sscanf(buf,"%d%*s",&messageCode);
			if(messageCode != ftpcode){
				printf("Unexpected response.\n", );
				exit(1);
			}
			firstChunk=0;
		}
		receivedBytes = readBuf(buf, sockfd);
	}
		return messageCode;
}

void writeBuf(char* buf, int sockfd){

	alarm(TIMEOUT);
	int reult = send(sockfd, buf, strlen(buf),0);
	alarm(0);
	return result;
}


void timeout()
{
	printf("\nConnection timed out!\n");
	exit(1);
}

int main(int argc, char** argv){
	int	sockfd, sockPasfd;
	char	buf[1024], readBuf[4097] = {0};
	int	bytes, receivedBytes;
	int status;
	char user[256], password[256], host[256], path[256], filename[256];

	if(argc != 2){

		printf("Usage: ./download <ftp_link>\n");
		exit(0);
	}

	parseUrl(argv[1], user, pass, host, path, filename);

	struct addrinfo hints;
	struct addrinfo *servinfo;

	memset(&hints, 0 , sizeof hints);

	hints.ai_family = AF_UNSPEC;

	hints.ai_socktype = SOCK_STREAM; // TCP stream sockets

	// get ready to connect
	if ((status = getaddrinfo(host, "21", &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
		exit(1);
	}

	sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

	/*connect to the server*/
    	if(connect(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) < 0){
        	perror("connect()");
			exit(1);
	}

	signal(SIGALRM, timeout);

	readMessage(readBuf, sockfd, 220); //220 Service ready for new user.
	sprintf(buf, "USER %s\r\n",user);
	writeBuf(buf,sockfd);

	readMessage(readBuf, sockfd, 331); //331	User name okay, need password.
	sprintf(buf,"PASS %s\r\n",password);
	writeBuf(buf,sockfd);

	readMessage(readBuf, sockfd, 230); //230	User logged in, proceed.
	
	writeBuf("PASV \r\n", sockfd); //PASV	Passive mode.

	readMessage(readBuf, sockfd, 227); //227 Entering Passive Mode <192,168, 50, 138, 249, 250>.

	char port[6], ip[15];
	getPasv(readBuf, port, ip);
	

}