// Anonymous Web Get
// Author: Aaron Smith, Alan Nash

#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "awget.h"

#define LENGTH 65536

void DieWithError(char *errorMessage){
	fprintf(stderr, "%s\n", errorMessage);
	exit(1);
}

void readChainFile(char* chainFile, ss_info_t* ssInfo){
	
	FILE *fp = NULL;
	fp = fopen(chainFile, "r");	
	char buffer[300];

	int ssCount = 0;
	fscanf(fp, "%d", &ssCount);
	fgets(buffer, 300, fp);
	printf("ssCount: %d\n", ssCount);

	int port = 0;
	char ipAddr[100];
	memset(&ipAddr, '\0', 100 * sizeof(char));
	
	ssInfo->ssCount = ssCount;
	ssInfo->ssPort = malloc(ssCount * sizeof(int));
	ssInfo->ssIP = malloc(ssCount);					

	int i;	
	for (i = 0; i < ssCount; i++){	
		fscanf(fp, "%s", ipAddr);		
		fscanf(fp, "%d", &port);	
		fgets(buffer, 300, fp);			
				
		ssInfo->ssIP[i] = malloc(100 * sizeof(char));
		strcpy(ssInfo->ssIP[i], ipAddr);			
		ssInfo->ssPort[i] = port;		
	}
	//fclose(fp);

}

void HandleClient(int clientSocket){
	
	char messageBuffer[500];
	memset(messageBuffer, '\0', sizeof(messageBuffer));
	int messageSize = recv(clientSocket, messageBuffer, 500, 0);
	
	if (messageSize < 0){
		DieWithError("receive failed");
	}

}

void createMessageFromStruct(ss_info_t* ssInfo, char* buffer){
			
	strcat(buffer, ssInfo->URL);
	strcat(buffer, ",");
	
	char temp[20];
	memset(&temp, '\0', sizeof(temp));
	sprintf(temp, "%d", ssInfo->ssCount);
	strcat(buffer, temp);
	strcat(buffer, ",");

	int i;	
	for (i = 0; i < ssInfo->ssCount; i++){
		strcat(buffer, ssInfo->ssIP[i]);
		strcat(buffer, ",");
	}

	int j;	
	for (j = 0; j < ssInfo->ssCount; j++){
		char temp[20];
		memset(&temp, '\0', sizeof(temp));
		sprintf(temp, "%d", ssInfo->ssPort[j]);
		strcat(buffer, temp);		
		strcat(buffer, ",");
	}

}

char* getFileName(char* URL){
		
	char* filename = malloc(100 * sizeof(char));	
	char *ptr;	

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

void chatClient(ss_info_t* ssInfo){

	struct sockaddr_in serverAddr;	
		
	int clientSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);	
	if (clientSocket < 0){
		DieWithError("client socket failed");
	}

	time_t t;
	int randSS;
	srand((unsigned) time(&t));
	randSS = rand() % ssInfo->ssCount;	

	char firstSSAddr[100];	
	strcpy(firstSSAddr, ssInfo->ssIP[randSS]);
	int firstSSPort = ssInfo->ssPort[randSS];
	
	printf("Request: %s\n", ssInfo->URL);
	printf("chainlist is\n");
	int i;	
	for (i = 0; i < ssInfo->ssCount; i++){
		printf("<%s, %d>\n", ssInfo->ssIP[i], ssInfo->ssPort[i]);
	}
	printf("next SS is <%s, %d>\n", firstSSAddr, firstSSPort);

	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = inet_addr(firstSSAddr);
	serverAddr.sin_port = htons(firstSSPort);	

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

	char buffer[500];
	memset(&buffer, '\0', sizeof(buffer));
	createMessageFromStruct(ssInfo, buffer);
	
	//printf("message send: %d\n", sizeof(buffer));		
	if (send(clientSocket, &buffer, sizeof(buffer), 0) != sizeof(buffer)){
		DieWithError("client send failed");
	} 	
			
	HandleClient(clientSocket);

	char* fr_name;
	fr_name = getFileName(ssInfo->URL);
	
	 /* Receive File from Server */ 
	int fr_block_sz;  
	char revbuf[LENGTH]; 
   	FILE *fr;
	fr = fopen(fr_name, "a");
	    while((fr_block_sz = recv(clientSocket, revbuf, LENGTH, MSG_WAITALL)) > 0)
	    {	
		//printf("%s\n", revbuf);					
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
	    //printf("Ok received from server!\n");
	  
        }
	fclose(fr);
	 
	printf("Received file %s\n", fr_name);
	printf("Goodbye!\n");
			
	close(serverSock);	

}

int checkURL(char* URL){

	char test[100];
	memset(&test, '\0', sizeof(test));	
	strncpy(test, URL, 11);	
	if (strcmp(test, "http://www.") != 0){
		memset(&test, '\0', sizeof(test));
		strncpy(test, URL, 4);
		if (strcmp(test, "www.") != 0){
			return -1;
		}	
	} 
	if (URL[strlen(URL)-1] == '/'){
		return -1;
	}	
	return 0;
}

int main(int argc, char *argv[]) {

	if (argc != 2 && argc != 4){
		fprintf(stderr, "Invalid number of arguments\n");
		return -1;
	}

	char URL[300];
	memset(&URL, '\0', sizeof(URL));	
	strcpy(URL, argv[1]);
		
	if (checkURL(URL) == -1){
		fprintf(stderr, "Invalid URL\n");
		return -1;
	}

	int cFlag = 0;
	char* chainFile = NULL;

	int c;		
	while ((c = getopt (argc, argv, "c:")) != -1){			
		switch (c){
			case 'c': cFlag = 1; chainFile = optarg; break;	
			case '?': fprintf(stderr, "Invalid flag\n"); return -1;
		}			
	}

	if (cFlag == 1 && chainFile == NULL){
		fprintf(stderr, "Invalid Chainfile Argument\n");
		return -1;
	} else{
		chainFile = "chaingang.txt";
	}
	
	ss_info_t *ssInfo = calloc(sizeof(ss_info_t), 0);
	ssInfo->URL = URL;

	readChainFile(chainFile, ssInfo);

 	chatClient(ssInfo);
	return 0;
}
