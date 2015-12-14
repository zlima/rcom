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
#include <string.h>

#define TIMEOUT 5

void parseUrl(char* buf,char* user, char* pass, char* host, char* path, char* filename){

	if(sscanf(buf, "ftp://%[^:]:%[^@]@%[^/]/%s", user, pass, host, path) < 4)
	{
		
		if(sscanf(buf,"ftp://%[^/]/%s", host, path) < 2) //no user
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

int readBuf(char*buf, int sockfd){
	alarm(TIMEOUT);
	int result= recv(sockfd, buf, 4096,0);
	alarm(0);
	return result;
}


int readMessage(char* buf, int sockfd, int ftpcode){
	int receivedBytes, firstChunk=1, messageCode;
	receivedBytes = readBuf(buf, sockfd);
	while(buf[receivedBytes-3]=='-' && receivedBytes > 0){
		buf[receivedBytes] = 0;
		if(firstChunk){
			sscanf(buf,"%d%*s",&messageCode);
			if(messageCode != ftpcode){
				printf("Unexpected response.\n");
				exit(1);
			}
			firstChunk=0;
		}
		receivedBytes = readBuf(buf, sockfd);
	}
		return messageCode;
}

int writeBuf(char* buf, int sockfd){

	alarm(TIMEOUT);
	int result = send(sockfd, buf, strlen(buf),0);
	alarm(0);
	return result;
}

void getPasv(char* buf, char* port, char* ip){
	
	int ip1, ip2, ip3, ip4, port1, port2;
	sscanf(buf,"%*d %*s %*s %*s (%d,%d,%d,%d,%d,%d)", &ip1, &ip2, &ip3, &ip4, &port1, &port2);
	int portVal = port1*256+port2;
	sprintf(ip, "%d.%d.%d.%d", ip1, ip2, ip3, ip4); //could be ipv6
	sprintf(port, "%d", portVal);
}


void timeout()
{
	printf("\nConnection timed out!\n");
	exit(1);
}

void getFile(char* buf, FILE* newFile, int fileSize, int sockfd){
	int numchunks =1;
	int receivedBytes, sumBytes =0;
	
	while(1){
		receivedBytes = readBuf(buf,sockfd);
		if(receivedBytes > 0){
			sumBytes += receivedBytes;
			fwrite(buf, sizeof(char), receivedBytes, newFile);
			printf("Chunk No:(%i) received!\n",numchunks);
			numchunks++;
		}
		else break;
	}
	
	if(sumBytes != fileSize){
		printf("Error receiving file!\n");
	}
	else printf("File received!\n");
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

	parseUrl(argv[1], user, password, host, path, filename);

	struct addrinfo hints;
	struct addrinfo *servinfo;

	memset(&hints, 0 , sizeof hints);

	hints.ai_family = AF_UNSPEC; //either ipv4 or ipv6

	hints.ai_socktype = SOCK_STREAM; // TCP stream sockets

	// get ready to connect
	if ((status = getaddrinfo(host, "21", &hints, &servinfo)) != 0) {//combina gethostbyname(3) e getservbyname(3), trata das dependencias de ipv4 e ipv6
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));//porta de serviço 23
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

	memset(&hints, 0, sizeof(hints));//clear hints struct
	
	hints.ai_family = AF_UNSPEC;//ipv4 ou ipv6
	
	hints.ai_socktype = SOCK_STREAM; //TCP stream socket
	
	//receber informaçao do socket do modo passivo
	getaddrinfo(ip,port,&hints,&servinfo);
	
	sockPasfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
	
	//connect no ip retornado para o passive mode
	if(connect(sockPasfd, servinfo->ai_addr, servinfo->ai_addrlen) < 0){
		perror("error connecting to pasFD");
		exit(1);
	}
	
	sprintf(buf, "retr %s\n",path);
	writeBuf(buf,sockfd);
	
	readMessage(buf, sockfd, 150);
	
	FILE * newFile = fopen(filename,"wb");//cria ficheiro 
	
	int fileSize;
	sscanf(buf,"%*[^(](%d%*s", &fileSize);
	
	getFile(readBuf,newFile, fileSize, sockPasfd);
	fclose(newFile);
	
	close(sockfd);
	close(sockPasfd);
	freeaddrinfo(servinfo);
	exit(0);

}