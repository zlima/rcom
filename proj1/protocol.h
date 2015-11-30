#pragma once

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

//defines trama
#define FLAG 0x7E
#define A_EMI 0x03
#define A_RCV 0x01
#define SET 0x07
#define UA 0x03
#define DISC 0x11
#define RR 0x01
#define REJ 0x05
#define BAUDRATE B9600
#define MODEMDEVICE "/dev/ttyS4"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

//alarm
#define ALARMTIMEOUT 3
#define ATTEMPTS 3


int alarmflag=1, conta=1, filesize=0;
int type;
char buf[255], writeFrame[255];
struct termios oldtio,newtio;
int fd;
int controlc =0;
int frameLength;
int tramas =0, resends=0, rejects=0;

void trigger();
void trigger2();
void alarm2setup();
void setAlarmFlag(int f);
int getAlarmFlag();
void alarmSetup();
int sendMessage(char * buffer, int length, int fd);
int statemachine(int fd, char * buf, int *res);
int readConFrame(int fd, int type);
int llopen(int porta, int flag);
int llclose(int fd);
int llwrite(int fd, char * packet, int length);
void settype(int type);
int shiftC();
int sendData(int fd, char c);
int stuffBytes(unsigned int numChars);
