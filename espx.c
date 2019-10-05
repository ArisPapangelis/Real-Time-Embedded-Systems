#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>

#include <ctype.h>
#include <sys/time.h>
#include <time.h>

#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 2288
#define SENDAEM 8883
#define MAX 278
#define BUFFLENGTH 2000
#define STUDENTS 4

void server(void);
void *client(void *); 
void *receiveMsg(void *);
//void *sendMsg(void *);
void produceMsg(int);

char IPs[][11]= { "10.0.88.58", "10.0.88.85", "10.0.88.96", "10.0.89.06" };
char **messageList= {"Today is a great day!", "Fancy grabbing a drink later?", "Wanna go to the gym?", "Eat clen tren hard test your limits."};
char buff[BUFFLENGTH][MAX];
int count=0;
int fullBuffer=0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char *argv[]){

	signal(SIGALRM,produceMsg);
	srand(time(NULL));
	
	produceMsg(0);
	

	pthread_t clientThread;
	
	pthread_create(&clientThread, NULL, client, NULL);
	
	
	
	server();
	
	return 0;
}


void server(void){
	int sockfd,new_socket[STUDENTS];
	struct sockaddr_in address;
	int addrlen = sizeof(address);
	
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed"); 
        exit(EXIT_FAILURE); 
    } 
	
	address.sin_family = AF_INET; 
    address.sin_addr.s_addr = INADDR_ANY; 
    address.sin_port = htons( PORT ); 
	
    if (bind(sockfd, (struct sockaddr *)&address,  sizeof(address))<0) {
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 
	
	if (listen(sockfd, 3) < 0) {
        perror("listen"); 
        exit(EXIT_FAILURE); 
    } 
	int i=0;
	for (;;){
		//printf("fuck alex wd\n");
		if ((new_socket[i] = accept(sockfd, (struct sockaddr *)&address,  (socklen_t*)&addrlen))<0) {
			perror("accept"); 
			exit(EXIT_FAILURE); 
		} 
		//printf("qawdqawd\n");
		pthread_t receiveThread;
		pthread_create(&receiveThread, NULL, receiveMsg, &new_socket[i]);
		
		i++;
		if (i>=STUDENTS) i=0;
		
	}
	
}

void *receiveMsg(void *newSock){
	//printf("qawdqawd\n");
	int *sock = (int *) newSock;
	
	char receivedMsg[278];
	
	recv(*sock, receivedMsg, sizeof(receivedMsg), 0);
	
	int i,doubleMsg,endIndex;
	while (strcmp(receivedMsg,"Exit")!=0){
		printf("%s",receivedMsg);
		pthread_mutex_lock(&mutex);
		doubleMsg=0;
		if (count>=2000){
			count=0;
			fullBuffer=1;
		}
		
		if (fullBuffer==0) {
			endIndex=count;
		}
		else {
			endIndex=BUFFLENGTH;
		}
		
		for (i=0; i<endIndex; i++){
			if (strcmp(receivedMsg, buff[i])==0){
				doubleMsg=1;
				break;
			}
		}
		if (doubleMsg==0){
			strcpy(buff[count], receivedMsg);
		}
		count++;
		pthread_mutex_unlock(&mutex);
		
		send(*sock,"OK",strlen("OK")+1,0);
		recv(*sock, receivedMsg, sizeof(receivedMsg), 0);
	}
	for (int i=0; i<count;i++){
		printf("%s\n",buff[i]);
	}
	
	close(*sock);
	pthread_detach(pthread_self());
	pthread_exit(NULL);
}


void *client(void *param){
    int sock = 0;
    struct sockaddr_in serv_addr; 


    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(PORT); 
    
	int j=0;
	while (j<STUDENTS){
		
		if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
		{ 
			printf("\n Socket creation error \n"); 
			pthread_exit(NULL); 
		} 
		// Convert IPv4 and IPv6 addresses from text to binary form 
		if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0)  
		{ 
			printf("\nInvalid address/ Address not supported \n"); 
		} 
	   
		if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
			printf("\nConnection Failed with IP %s\n", IPs[j]);
		} 
		else {
			//pthread_t sendThread;
			//pthread_create(&sendThread,NULL,sendMsg,&sock);
			
			pthread_mutex_lock(&mutex);
			int endIndex;
			if (fullBuffer==0) {
				endIndex=count;
			}
			else {
				endIndex=BUFFLENGTH;
			}
			char ack[3];
			
			for (int i=0; i<endIndex;i++){
				send(*sock, buff[i], strlen(buff[i])+1, 0);
				recv(*sock,ack,sizeof(ack),0);
			}
			send(*sock, "Exit", strlen("Exit")+1,0);
			pthread_mutex_unlock(&mutex);			
		}
		close(sock);
		j++;
		if (j==STUDENTS){
			j=0;
			sleep(5);
		}
	}
	
	return NULL;
}


/*
void *sendMsg(void *newSock){
	
	int *sock = (int *) newSock;
	

	
	//close(*sock);
	pthread_detach(pthread_self());
	pthread_exit(NULL);
	
}
*/

void produceMsg(int sig){
	
	pthread_mutex_lock(&mutex);
	
	char message[278];
	unsigned int sender= 8883;
	
	char receiver[5];
	int rS= rand()%4;
	receiver = { IPs[rS][6], IPs[rS][7], IPs[rS][9], IPs[rS][10], '\0'};
	
	sprintf(message, "%d_%s_%d_%s",sender, receiver, (unsigned)time(NULL), messageList[rand()%4]);
	
	if (count>=2000){
		count=0;
		fullBuffer=1;
	}	
	strcpy(buff[count],message);
	count++;
		
	alarm(rand() % (5*60 + 1 - 60) + 60);
	
	pthread_mutex_unlock(&mutex);

	
}
