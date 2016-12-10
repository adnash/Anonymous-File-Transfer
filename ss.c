// Project 2: Anonymous Web Get
// Author: Aaron Smith, Alan Nash
// Date:   10/1/2015
// Class:  CS457
// Email:  acsmit@rams.colostate.edu

#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define MAXPENDING 5
#define LENGTH 65536

typedef struct ss_info {

	char* URL;
	int ssCount;
	char** ssIP;
	int* ssPort;

} ss_info_t;

void DieWithError(char *errorMessage){
	fprintf(stderr, "%s\n", errorMessage);
	exit(1);
}

char* getLocalIP(){
	
	char *ipAddr;	
	struct ifaddrs *addrHead, *addrPoint;
	int family, nameValue;
	char host[NI_MAXHOST];

	if (getifaddrs(&addrHead) < 0) {
		DieWithError("Error getting IP adress");
	}

	for (addrPoint = addrHead; addrPoint != NULL; addrPoint = addrPoint->ifa_next) {
		family = addrPoint->ifa_addr->sa_family;
		if (family == AF_INET) {
			nameValue = getnameinfo(addrPoint->ifa_addr, sizeof(struct sockaddr_in),
				                       host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
			if (nameValue == -1) {
				DieWithError("Error getting address info");
			}
			if (strcmp(addrPoint->ifa_name, "em1") == 0){
				ipAddr = host;
				break;
			}				
		}
	}	
	return ipAddr;
}

char* getFileName(char* buffer){
	
	char URL[300];
	memset(&URL, '\0', sizeof(URL));
	char* filename = malloc(100 * sizeof(char));		
	int index;
	char *ptr;	

	ptr = strchr(buffer, ',');
	index = ptr - buffer;

	memcpy(URL, &buffer[0], index);

	ptr = strrchr(URL, '/');
	while (1){	
		if (ptr == NULL){
			filename = "index.html";
			break;
		} else if (ptr-URL == strlen(URL)-1){					
			URL[strlen(URL)-1] = '\0';
			ptr = strrchr(URL, '/');					
		} else{	
			memcpy(filename, ptr+1, strlen(URL));
			break;
		}	
	}
	return filename;

}

void SteppingClient(char* buffer, char* nextIP, int nextPort){    
        
	struct sockaddr_in serverAddr;

	int clientSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);	
	if (clientSocket < 0){
		DieWithError("client socket failed");
	}

	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = inet_addr(nextIP);
	serverAddr.sin_port = htons(nextPort);	

	if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0){
		DieWithError("client connect failed");
	}	

	int serverLength;
	int serverSock;
	serverLength = sizeof(serverAddr);
	serverSock = getsockname(clientSocket, (struct sockaddr *)&serverAddr, &serverLength);
	
	if (serverSock < 0){
		DieWithError("get server sock failed");
	}

	printf("waiting for file...\n");
	printf("...\n");							
	
	if (send(clientSocket, buffer, sizeof(buffer)+500, 0) != sizeof(buffer)+500){
		DieWithError("client send failed");
	}
        
	
        /* Receive File from Server */ 
	int fr_block_sz;  
	char revbuf[LENGTH]; 
	char* fr_name;
	fr_name = getFileName(buffer);
	FILE *fr;
	fr = fopen(fr_name, "a");	
	    while((fr_block_sz = recv(clientSocket, revbuf, LENGTH, MSG_WAITALL)) > 0){			
		if(fr == NULL){
			printf("File %s Cannot be opened.\n", fr_name);
		} else{
			int write_sz = fwrite(revbuf, sizeof(char), fr_block_sz, fr);
			if(write_sz < fr_block_sz)
				{
			    error("File write failed.\n");
			}
			bzero(revbuf, LENGTH);
			if (fr_block_sz == 0 || fr_block_sz != 65536) 
			{
				break;
			}
		}
		if(fr_block_sz < 0){
			if (errno == EAGAIN)
			{
				printf("recv() timed out.\n");
			}
			else
			{
				fprintf(stderr, "recv() failed due to errno = %d\n", errno);
			}
		}
	   
        }
	 fclose(fr);
}

void ParseMessage(char* buffer, char* address, int port){

        char ssCount[4];
	int newCount;
	char URL[300];
	memset(&URL, '\0', sizeof(URL));	

	int commaIndex;
	int nextCommaIndex;
	char *ptr;
	char *ptr2;

	ptr = strchr(buffer, ',');
	commaIndex = ptr - buffer;

	memcpy(URL, &buffer[0], commaIndex);

	ptr2 = strchr(ptr+1, ',');
	nextCommaIndex = ptr2 - buffer;	

	memcpy(ssCount, &buffer[commaIndex+1], nextCommaIndex - commaIndex - 1);
	ssCount[nextCommaIndex - commaIndex] = '\0';
	newCount = atoi(ssCount)-1;
	char** ssIP;
	char** ssPort;
	ssPort = malloc(atoi(ssCount));
	ssIP = malloc(atoi(ssCount));		

	int i;
	for (i = 0; i < atoi(ssCount); i++){
		ptr = ptr2;
		commaIndex = ptr - buffer;	

		ptr2 = strchr(ptr+1, ',');
		nextCommaIndex = ptr2 - buffer;	
		
		char temp[100];	
		memset(&temp, '\0', sizeof(temp));	
		memcpy(temp, &buffer[commaIndex+1], nextCommaIndex - commaIndex - 1);		
		
		ssIP[i] = malloc(100 * sizeof(char));
		strcpy(ssIP[i], temp);
	}

	for (i = 0; i < atoi(ssCount); i++){
		ptr = ptr2;
		commaIndex = ptr - buffer;	

		ptr2 = strchr(ptr+1, ',');
		nextCommaIndex = ptr2 - buffer;	

		char temp[100];	
		memset(&temp, '\0', sizeof(temp));	
		memcpy(temp, &buffer[commaIndex+1], nextCommaIndex - commaIndex - 1);		

		ssPort[i] = malloc(100 * sizeof(char));
		strcpy(ssPort[i], temp);
		
	}

	printf("Request: %s\n", URL);

        if(atoi(ssCount) != 1){
            printf("chainlist is\n");
            int indexOfCurrent;	
            for (i = 0; i < atoi(ssCount); i++){
		if (strcmp(ssIP[i], address) != 0 || atoi(ssPort[i]) != port){			
			printf("<%s, %s>\n", ssIP[i], ssPort[i]);
		} else{
		    indexOfCurrent = i;
                }
            }
            time_t t;
            int randSS;
            srand((unsigned) time(&t));
            randSS = rand() % newCount;	
            while (randSS == indexOfCurrent){		
            	randSS = rand() % atoi(ssCount);
            }
            printf("next SS is <%s, %s>\n", ssIP[randSS], ssPort[randSS]);	

            ptr = strchr(buffer, ',');
            commaIndex = ptr - buffer;
            buffer[commaIndex+1] = '\0';

            char temp[20];
            memset(&temp, '\0', sizeof(temp));
            sprintf(temp, "%d", newCount);
            strcat(buffer, temp);
            strcat(buffer, ",");
		
            for (i = 0; i < atoi(ssCount); i++){
		if (i != indexOfCurrent){		
			strcat(buffer, ssIP[i]);
			strcat(buffer, ",");
                }
            }

            int j;	
            for (j = 0; j < atoi(ssCount); j++){			
                if (j != indexOfCurrent){		
                    strcat(buffer, ssPort[j]);		
                    strcat(buffer, ",");
                }
            }
            SteppingClient(buffer, ssIP[randSS], atoi(ssPort[randSS]));
        }else{
		char* filename;
		filename = getFileName(buffer);
	
		printf("Request: %s\n", URL);		
		printf("chainlist is empty\n");
		printf("issuing wget for file %s\n", filename);
		printf("...\n");
            	char command[100] = "wget ";
            	strcat(command, URL);
            	system(command);
		printf("File received\n");
        }
}

char* HandleClient(int clientSocket, char* address, int port){
	
	char messageBuffer[500];
	memset(messageBuffer, '\0', sizeof(messageBuffer));
	int messageSize = recv(clientSocket, messageBuffer, 500, 0);
	if (messageSize < 0){
		DieWithError("receive failed");
	}

	char* filename = getFileName(messageBuffer);
	
	ParseMessage(messageBuffer, address, port);	
			
	return filename;
}

void SteppingServer(int portNum){

	struct sockaddr_in serverAddr;
	struct sockaddr_in clientAddr;		
	
	int serverSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (serverSocket < 0){
		DieWithError("server socket failed");
	}

	char *ipAddr = getLocalIP();	

	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	inet_aton(ipAddr, &serverAddr.sin_addr);
	if (portNum != 0){
		serverAddr.sin_port = htons(portNum);
	} else{
		serverAddr.sin_port = 0;
	}

	if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0){
		DieWithError("server bind failed");
	} 

	while (1){	

	if (listen(serverSocket, MAXPENDING) < 0){
		DieWithError("server listen failed");
	} 

	socklen_t len = sizeof(serverAddr);
	int num = getsockname(serverSocket, (struct sockaddr *)&serverAddr, &len);
	char address[100];
	const char* localAddr = inet_ntop(AF_INET, &serverAddr.sin_addr, address, 100);

	printf("Waiting for a connection on %s port %d\n", address, ntohs(serverAddr.sin_port)); 
	
	int clientLength;
	int clientSock;
	clientLength = sizeof(clientAddr);
	
	fd_set socketSet;	
	int maxSocket;
	char* fs_name;
	int maxSockets = 10;
	int socketArray[10];
	int j;
	for (j = 0; j < 10; j++){
		socketArray[j] = 0;
	}

	FD_ZERO(&socketSet);
	FD_SET(serverSocket, &socketSet);
	maxSocket = serverSocket;

	while (1){
		int i;
		for (i = 0; i < maxSockets; i++){
			if (socketArray[i] > 0){
				FD_SET(socketArray[i], &socketSet);
			}
			if (socketArray[i] > maxSocket){
				maxSocket = socketArray[i];
			}
		}				
		
		int selectValue = select(maxSocket+1, &socketSet, NULL, NULL, NULL);		
		
		if (FD_ISSET(serverSocket, &socketSet)){			
			clientSock = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientLength);
			if (clientSock < 0 && (errno!=EINTR)){
				DieWithError("server accept failed");
			}
			
			for (i = 0; i < maxSockets; i++){
				if (socketArray[i] == 0){
					socketArray[i] = clientSock;
					break;
				}
			}
	
		}
		for (i = 0; i < maxSockets; i++){
			if (FD_ISSET(socketArray[i], &socketSet)){		
				fs_name = HandleClient(clientSock, address, ntohs(serverAddr.sin_port));
				char sdbuf[LENGTH]; // Send buffer
				printf("Relaying file...\n");
				FILE *fs = fopen(fs_name, "r");
				if(fs == NULL)
				{
				    printf("ERROR: File %s not found on server.\n", fs_name);
						exit(1);
				}

				bzero(sdbuf, LENGTH); 
				int fs_block_sz;
				fseek(fs, SEEK_SET, 0); 
				send(clientSock, sdbuf, 512, 0);
				while((fs_block_sz = fread(sdbuf, sizeof(char), LENGTH, fs))>0)
				{
				    //printf("%s", sdbuf);
				    if(send(clientSock, sdbuf, fs_block_sz, 0) < 0)
				    {
					perror("write failed");
				    }
				    bzero(sdbuf, LENGTH);
				}

				printf("Goodbye!\n");
				remove(fs_name);     
				close(clientSock);
				socketArray[i] = 0;				
			}
		}
		int checkConnect = 0;
		for (i = 0; i < maxSockets; i++){
			if (socketArray[i] != 0){
				checkConnect = 1;
			}
		}
		if (checkConnect == 0){
			break;
		}	

	}
				
	
	}
}

int main(int argc, char *argv[]) {
    if (argc != 3 && argc != 1){
		fprintf(stderr, "Invalid number of arguments\n");
		return -1;
	}

	unsigned short portNum = 0;

	int pFlag = 0;

	int c;		
	while ((c = getopt (argc, argv, "p:")) != -1){			
		switch (c){
			case 'p': pFlag = 1; portNum = atoi(optarg); break;
			case '?': fprintf(stderr, "Invalid flag\n"); return -1;
		}			
	}
        
        SteppingServer(portNum);	
	
	return 0;

}
