/*Non-Canonical Input Processing*/
// ttys4      ttys0
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>


#define BAUDRATE B9600
#define MODEMDEVICE "/dev/ttyS4"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
#define RESEND_TIMER 3



#define FILE_CHUNK_SIZE 255



int c, res, filesize, type;

int i, sum = 0, speed = 0, resendretries=0, n=0;



struct sigaction action;

volatile int STOP=FALSE;


FILE * file=NULL;
char * filename;
char name[255];
char * filedata;
int frameLength;

int openfile(char * filenm);
int createControlPackage(char *cntrpck, int packetcontrol,unsigned int filesize,char * filename);
void sendDataPackets(int fd);
void sendDataPacket(int fd, char *packet, int chunkSize, unsigned char chunkNo);
void sendFile(int fd);
int readPacket(char * buffer);
void readapplication(int fd,char * chunk);
