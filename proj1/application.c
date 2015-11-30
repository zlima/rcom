#include "application.h"

int openfile(char * filenm){
int result;
	file = fopen(filenm,"r");
	if(file != NULL){

		fseek(file, 0, SEEK_END); // seek to end of file
		filesize = ftell(file); // get current file pointer
		fseek(file, 0, SEEK_SET);

		filename = filenm;
		filedata = (char*)malloc(sizeof(char)*filesize);
		result = fread (filedata,1,filesize,file);


	if (result != filesize) {fputs ("Reading error",stderr); exit (3);}
		fclose(file);
		return TRUE;
	} 
	else return FALSE;
}

void sendDataPacket(int fd, char *packet, int chunkSize, unsigned char chunkNo){
	//cabeçalho do pacote de dados
	packet[0] = 0; //C
	packet[1] = chunkNo; //N
	packet[2] = chunkSize/256; //l1
	packet[3] = chunkSize%256; // L2
	//k = 256*L1+L2

	printf("WRITTING DATA PACKET No: %i\n", (int)chunkNo);
	llwrite(fd,packet,chunkSize+4);
}


void sendDataPackets(int fd)
{
	int i;
	unsigned char n;
	n=0;//numero da chunk

	char chunkBuf[FILE_CHUNK_SIZE+4]; //buf com o chunk a enviar + cabeçalho

	for(i=0;i<(filesize/FILE_CHUNK_SIZE);i++){
		memcpy(chunkBuf+4, &filedata[i*FILE_CHUNK_SIZE],FILE_CHUNK_SIZE);//copia a data de um chunk para o bufer
		sendDataPacket(fd,chunkBuf,FILE_CHUNK_SIZE,n);

		if(n==255)
			n=0;
		else n++;
	}

	if(filesize%FILE_CHUNK_SIZE>0){
		memcpy(chunkBuf+4,&filedata[i%FILE_CHUNK_SIZE],filesize%FILE_CHUNK_SIZE);//copiar o resto dos bytes
		sendDataPacket(fd,chunkBuf,filesize%FILE_CHUNK_SIZE,n);
	}


		printf("TAMANHO DA PUTA EM BOCADOS: %i\n",filesize);
	
}

void sendFile(int fd){
	int length;
	char buf[255];
	length = createControlPackage(buf,1,filesize,filename);

	printf("WRITTING START CONTROL PACKET\n");
	llwrite(fd,buf,length);
	

	sendDataPackets(fd);

sleep(1);
	length = createControlPackage(buf,2,filesize,filename);
	printf("WRITTING END CONTROL PACKET\n");
	llwrite(fd,buf,length);
	
}


int writeapplication(char * filen,int fd){

	if(openfile(filen)==TRUE){

		sendFile(fd);
	//char fileName[100];
	//int i;
	//for(i=0;i<20;i++){
	//fileName[i]=buf[i+3];


	}else
	{
		printf("Falha a abrir ficheiro!\n");
	}


}

int readPacket(char * buffer){
    unsigned char packetc=buffer[0];
int k;

    if(packetc==1){ //START
        unsigned char t1 = buffer[1], l1 = buffer[2], i, tmp = l1;
            for(i=3;i<l1+3;++i)
                filesize |= (((unsigned char)buffer[i]) << (8*--tmp));
                    
            unsigned char t2 = buffer[i++], l2 = buffer[i++], j=i;

            for(;i<l2+j;i++){

                name[i-j]=buffer[i];


}




    }else if(packetc==2){//END
    	return 0;
    }else if(packetc==0){//DATA

        if(buffer[1]==n){
            unsigned char l2= buffer[2], l1 = buffer[3];
            int k = 256*l2+l1;
	    
            fwrite(buffer+4, sizeof(char), k, file);
            if(n==255){
                n=0;
            }else{
                n++;
            }
        }
    }
    return 1;
}






int createControlPackage(char *cntrpck, int packetcontrol,unsigned int filesize,char * filename)
{

	cntrpck[0]=packetcontrol; //C
	cntrpck[1]=0; //T1 - tamanho do ficheiro
	int i=3, currentByte, significant = 0;

	for(currentByte=sizeof(filesize);currentByte>0;currentByte--) //percorre inteiro byte a byte do mais significativo para o menos
	{
		unsigned char byte = (filesize >> (currentByte-1)*8) & 0xff; //passa bytes individuais(octetos) para controlPacket
		
		if(!significant) //enquanto forem não significativos
			if(byte!=0) //se byte actual não for zero
				significant = 1; //todos os seguintes são significativos
		if(significant) //para todos os bytes significativos, enviar
			cntrpck[i++]=byte; //V1
	}

	cntrpck[2]=i-3; //L1
	cntrpck[i++] = 1; //T2 - nome do ficheiro
	cntrpck[i++] = strlen(filename); //L2
	
	int j;
	for(j=0;j<strlen(filename);j++){ //escreve caracteres individuais do nome do ficheiro
		cntrpck[i++] = filename[j]; //V2
	}



return i; //retorna o tamanho do pacote de controlo
}





int main(int argc, char** argv){
	int i, sum = 0, speed = 0;
	int fd;
	if ( (argc < 3)||((strcmp("/dev/ttyS0", argv[1])!=0)&&(strcmp("/dev/ttyS4", argv[1])!=0))||((atoi(argv[2])!=0) && (atoi(argv[2])!=1))) {
		printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS4 TRANSMITTER|RECEIVER\n");
		exit(1);
	}
	type = atoi(argv[2]);
    //sprintf(type,"%c",argv[2]);

	settype(type);
	int porta =-1;
	if(strcmp(argv[1],"/dev/ttyS0")==0){
		porta =0;
	}else if(strcmp(argv[1],"/dev/ttyS4")==0){
		porta = 4;
	}

	
	fd= llopen(porta,type);

	if(type==1){

		writeapplication(argv[3],fd);	
	}else{
		file = fopen ( "tmp" , "wb" );
		char chunk[FILE_CHUNK_SIZE+5];

		readapplication(fd, chunk);
		fclose(file);
	
		rename("tmp", name);
	}
	llclose(fd);
	//sleep(1);
    /*
    O ciclo WHILE deve ser alterado de modo a respeitar o indicado no guião
    */

    return 1;
}

void readapplication(int fd, char * chunk){



	int res =-1, response=1;
	while(response!=0){
//printf("continua\n");

		res=llread(fd,chunk);

		if(res!=-1){
			response=readPacket(chunk);	
			//printf("ReadPacket Response: %i \n",response);		
		}
	}

}
