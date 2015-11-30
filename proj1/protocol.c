#include "protocol.h"

void trigger(){
    alarmflag++;
    resends++;
    if(alarmflag < ATTEMPTS){
        printf("Resending frame. Tries count = %i\n",alarmflag);
        sendMessage(buf,5,fd);
        alarm(3);
    }else{
        printf("alarm timeout\n");
        exit(1);
    }
}
void setAlarmFlag(int f){
    alarmflag = f;
}
int getAlarmFlag(){
    return alarmflag;
}
void alarmSetup() {
    (void) signal(SIGALRM, trigger);
}

void alarm2Setup(){
    (void) signal(SIGALRM,trigger2);
}

void trigger2(){
    alarmflag++;
    if(alarmflag < ATTEMPTS){
        printf("Error.. Resending frame. Tries count = %i\n",alarmflag);
        sendMessage(writeFrame,frameLength,fd);
        alarm(ALARMTIMEOUT);
    }else{
        printf("alarm timeout\n");
        exit(1);
    }
}

int sendMessage(char* buffer, int length, int fd){

    int sent_size = write(fd, buffer, length);
    if (sent_size == length)
        return 1;
    else
        return 0;
}

int statemachine(int fd, char * buffer, int *res){ 
    int state = 0, bcc2=0, i=0; 
    char A; 
    char C; 

    //type = 1 emissor 
    while(getAlarmFlag()<3){ 

        if(read(fd,buf,1)){ 
            //printf("STATE: %d\n",state); 
            //printf("BUF: 0x%x\n",buf[0]); 
            switch(state){ 
                case 0://FLAG 
                if(buf[0] == FLAG){ 
                    state= 1; 
                }else{ 
                    state= 0; 
                } 
                break; 

                case 1: //A 
                if(buf[0] == A_EMI||buf[0] == A_RCV){ 
                    A= buf[0]; 
                    state= 2; 
                }else if (buf[0] == FLAG){ 
                    state= 1; 
                }else state= 0; 
                break; 

                case 2: //C  
                if(buf[0] == FLAG){ 
                    state= 1; 
                    state= 3; 
                }else if((buf[0]==SET && type==0)||(buf[0]==UA)||(buf[0] == DISC)|| (buf[0]==((0x00)^(0<<5)))||(buf[0]== ((0x00)^(1<<5)))|| (buf[0]==1)||(buf[0]== (RR^(1<<5)))|| (buf[0]==(RR^(0<<5)))||(buf[0]== (REJ^(0<<5)))||(buf[0]== (REJ^(1<<5)))){ 
                    C= buf[0]; 
                    state= 3; 
                }else state= 0; 
                break; 

                case 3: //BCC 
                if(buf[0] == A^C){ 
                    state= 4; 
                }else if (buf[0] == FLAG){ 
                    state= 1; 
                }else state= 0; 
                break; 

                case 4: //F  
                if(buf[0] == FLAG){ //control frame
                    if(C==(controlc<<5)){ 
                        tramas++;			         
                        *res = i-1; 
                        if(buffer[i-1]==bcc2){ 

                            controlc=shiftC();     
                        }else{ 
                            return -1; 
                        } 
                    }

                    return C; 
                }else{//data frame 

                    //falta destuffing
            if(buf[0]==0x7d) //se caracter for 0x7d, destuff
            {
                char byte;
                    read(fd, buf, 1); //lê o próximo
                    byte=buf[0];
                    if(byte == 0x5e)
                        byte = 0x7e;
                    else if(byte == 0x5d)
                        byte = 0x7d;
                    buffer[i]= byte;
                }else{
                    buffer[i]= buf[0];
                }                     
                if(i>0){ 
                    bcc2^=buffer[i-1]; 
                } 
                i++; 
            } 
            break; 
        } 
    } 

} 
return -1;  
}

int stuffBytes(unsigned int numChars)
{
    int i=4; //avança os quatro primeiros (cabeçalho da trama)
    for(;i<numChars+4;i++)
    {
        if(writeFrame[i]==0x7e)
        {
            memmove(writeFrame+i+1,writeFrame+i,numChars-(i-4)); //avança todos os caracteres uma posição
            writeFrame[i]=0x7d;
            writeFrame[++i]=0x5e;
            numChars++; //há mais um caracter a transmitir
        }
        else if(writeFrame[i]==0x7d)
        {
            memmove(writeFrame+i+1,writeFrame+i,numChars-(i-4)); //avança todos os caracteres uma posição
            writeFrame[i]=0x7d;
            writeFrame[++i]=0x5d;
            numChars++; //há mais um caracter a transmitir
        }
    }
    return numChars; //retorna novo número de caracteres a transmitir
}    


int readConFrame(int fd, int type){
    int res, responseC = statemachine(fd,buf, &res);
    if(responseC== type){//change conditions
        return 1;
    }else return 0;
}



int sendData(int fd, char c){
    //construir cabeçalho da trama
    writeFrame[0] = FLAG; //flag
    writeFrame[1] = A_EMI; //A
    writeFrame[2] = c; // C
    writeFrame[3] = writeFrame[1] ^ writeFrame[2]; //BCC1
    writeFrame[4] = FLAG;
    write(fd, writeFrame, 5); 
}

int llwrite(int fd, char * packet, int length){
	//construir cabeçalho da trama
	writeFrame[0] = FLAG; //flag
	writeFrame[1] = A_EMI; //A
	writeFrame[2] = ((0x00) ^ (controlc<<5)); // C
	writeFrame[3] = writeFrame[1] ^ writeFrame[2]; //BCC1 

    int i,bcc2=0;
    for(i=4; i<length+4;++i){

        writeFrame[i] = packet[i-4];
        bcc2 ^= writeFrame[i];
    }

    writeFrame[i++] = bcc2;
    int packetCharsAfterStuffing = stuffBytes(length+1); //Número de caracteres para fazer stuffing
    i=packetCharsAfterStuffing+4; //posição no índice seguinte é igual ao novo número de caracteres a transmitir + cabeçalho da trama
    
    writeFrame[i++] = FLAG;



    frameLength = i;

    printf("SENDING DATA FRAME\n");
    alarm2Setup();
    setAlarmFlag(0);
    alarm(ALARMTIMEOUT);
    if(sendMessage(writeFrame,frameLength,fd)==TRUE){
        //read à espera da conf

        if(statemachine(fd,NULL,NULL)==(RR^(shiftC()<<5))){
            controlc = shiftC();
            printf("RECEIVER READY!\n");
        }else{
            printf("FRAME REJECTED... Resending..\n");
            llwrite(fd,packet,length);
        }
    }
    else{ //nao recebe conf
        rejects++;
        printf("ERROR SENDING DATA FRAME\n");
        llwrite(fd,packet,length);
    }
    tramas++;
    return length;

}

int llopen(int porta, int flag){
    char * cporta;
    printf("porta : %d \n",porta);
    if(porta ==0){
        cporta = "/dev/ttyS0";}
        else{
            cporta = "/dev/ttyS4";
        }
        printf("%s n",cporta);
        fd = open(cporta, O_RDWR | O_NOCTTY );
        if (fd <0) {perror(cporta); exit(-1); }
    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
        perror("tcgetattr");
        exit(-1);
    }
    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = OPOST;
    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 0;  /* inter-character timer unused */
    newtio.c_cc[VMIN] = 1;  /* blocking read until 5 chars received */
    tcflush(fd, TCIFLUSH);
    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }
    alarmSetup();
    alarm(0);
    if(flag==1){ //transmitor
        int tries = 0;
        buf[0] = FLAG;
        buf[1] = A_EMI;
        buf[2] = SET;
        buf[3] = A_EMI^SET;
        buf[4] = FLAG;
        if(sendMessage(buf,5,fd) == FALSE){
            printf("ERROR SENDING SET COMMAND. WILL NOW EXIT\n");
            exit(1);
        }
        else{
            setAlarmFlag(0);
            alarm(ALARMTIMEOUT);
            if(readConFrame(fd,UA) == TRUE){
                alarm(0);
                printf("CONNECTION ESTABLISHED\n");
            }
        }
    }
    else if(flag==0){//receiver
        int i = 0;
        /* loop for input */
        int state=0;
        /* returns after 5 chars have been input */
        setAlarmFlag(0);
        if(readConFrame(fd,SET) == TRUE){
        }
        int j=0;
        printf("Saiu\n");
        buf[0] = FLAG;
        buf[1] = A_RCV;
        buf[2] = UA;
        buf[3] = buf[1]^buf[2];
        buf[4] = FLAG;
        int res2 = write(fd,buf,10);
        sleep(1);
        j =0;
        for(j; j<5;j++){
            printf("Message: 0x%x\n", buf[j]);
        }
        sleep(1);
    }
    return fd;
}




int llclose(int fd){

    if(type==1){ //transmitor
        int tries = 0;
        buf[0] = FLAG;
        buf[1] = A_EMI;
        buf[2] = DISC;
        buf[3] = buf[1]^buf[2];
        buf[4] = FLAG;
        if(sendMessage(buf,5,fd) == TRUE){
            if(readConFrame(fd,DISC) == TRUE){
                alarm(0);
                printf("DISC received\n");
                buf[0] = FLAG;
                buf[1] = A_EMI;
                buf[2] = UA;
                buf[3] = buf[1]^buf[2];
                buf[4] = FLAG;
                if(sendMessage(buf,5,fd) == TRUE){
                  printf("Disconnecting...\n");
              }
          }

      }
  }
    else if(type==0){//receiver

        /* loop for input */

        /* returns after 5 chars have been input */
        setAlarmFlag(0);
        if(readConFrame(fd,DISC) == TRUE){
        }
        int j=0;

        buf[0] = FLAG;
        buf[1] = A_RCV;
        buf[2] = DISC;
        buf[3] = buf[1]^buf[2];
        buf[4] = FLAG;
        int res2 = write(fd,buf,10);
        readConFrame(fd,UA);
    }
    sleep(2);
    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      return -1;
  }

  close(fd);
  printf("Tramas: %i \n", tramas);
  printf("Resends: %i \n", resends);
  printf("Rejects: %i \n", rejects);

}

void settype(int t){
	type =t;
}

int shiftC(){
    if(controlc==0){
        return 1;
    }else if(controlc==1){
        return 0;
    }

    
}

int llread(int fd, char * buffer){
    int res;
    int responseC= statemachine(fd, buffer,&res);
    if(responseC== DISC){
        return 0;
    }else if (responseC==-1){
        rejects++;
        sendData(fd,REJ ^ (controlc<<5) );
        return -1;
    }else{

        sendData(fd,RR ^ (controlc<<5) );
        return res;
    }
}
