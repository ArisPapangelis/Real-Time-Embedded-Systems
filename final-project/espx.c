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
#define SENDER 8906
#define MAX_MSG_LENGTH 278
#define BUFFLENGTH 2000

void server(void);
void *client(void *); 
void *receiveMsg(void *);
//void *sendMsg(void *);
void produceMsg(int);
void catch_int(int);
void catch_term(int);
void sendMsgs(int, int);

char **IPs;
int ip_count;

int *IPsLastMsgSentIndex;
char **IPsLastMsgSent;
int *IPsToAEMs;

char **messageList;
int message_count;

char buff[BUFFLENGTH][MAX_MSG_LENGTH];
int count=0;
int fullBuffer=0;

pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t fd_mutex = PTHREAD_MUTEX_INITIALIZER;

void initialize_addresses(char *file){
	FILE *fp;
	int i;

	fp = fopen(file, "r");
	
	fscanf(fp, "%d\n", &ip_count);
	IPsLastMsgSentIndex = (int *)malloc(ip_count * sizeof(int));
	IPsToAEMs =(int *)malloc(ip_count * sizeof(int));  

	IPs = (char **)malloc(ip_count * sizeof(char *)); 
	IPsLastMsgSent = (char **)malloc(ip_count * sizeof(char *));

    for (i = 0; i < ip_count; i++){
		IPsLastMsgSentIndex[i] = -1;
        IPsLastMsgSent[i] = (char *)malloc(MAX_MSG_LENGTH * sizeof(char));
        strcpy(IPsLastMsgSent[i], "null");
		IPs[i] = (char *)malloc(50 * sizeof(char));
		fscanf(fp, "%s %d\n", IPs[i], &IPsToAEMs[i]);
		IPs[i][strcspn(IPs[i], "\n")] = '\0';

	} 
	fclose(fp);
}

void initialize_messages(char *file){
	FILE *fp;
	int i;

	fp = fopen(file, "r");
	
	fscanf(fp, "%d\n", &message_count);
	messageList = (char **)malloc(message_count * sizeof(char *)); 

    for (i = 0; i < message_count; i++){
        messageList[i] = (char *)malloc(50 * sizeof(char));
		fgets(messageList[i], 50, fp);
		messageList[i][strcspn(messageList[i], "\n")] = '\0';

	} 
	fclose(fp);
	for(int i = 0 ; i < message_count ; i++){
		printf("%s\n", messageList[i]);
	}
}

int main(int argc, char *argv[]){
	printf("%d\n", argc);
	if(argc < 3){
		printf("Not enough arguments given");
		exit(-1);
	}

	// populate IPs and messageList
	initialize_addresses(argv[1]);
	initialize_messages(argv[2]);
	
	printf("MAIN:\tSetting up sig handlers...\n");
	signal(SIGALRM,produceMsg);
	signal(SIGTERM, catch_term);
	signal(SIGINT, catch_int);
	srand(time(NULL));

	printf("MAIN:\tIniatilizing buffer...\n");
	produceMsg(0);

	pthread_t clientThread;	
	pthread_create(&clientThread, NULL, client, NULL);
	
	printf("MAIN:\tStarting server...\n");
	server();
	
	return 0;
}

void catch_int(int signal){
	printf("INT signal ...\n");
	for(int i = 0; i < message_count; i++){
		free(messageList[i]);
	}
	free(messageList);

	for(int i = 0; i < ip_count; i++){
		free(IPs[i]);
		// free(IPsLastMsgSentIndex[i]);
	}
	free(IPs);
	free(IPsToAEMs);
	free(IPsLastMsgSent);
	free(IPsLastMsgSentIndex);
	exit(1);
}

void catch_term(int signal){
	printf("KILL signal ...\n");
	for(int i = 0; i < message_count; i++){
		free(messageList[i]);
	}
	free(messageList);

	for(int i = 0; i < ip_count; i++){
		free(IPs[i]);
		// free(IPsLastMsgSentIndex[i]);
	}
	free(IPs);
	free(IPsToAEMs);

	free(IPsLastMsgSent);
	free(IPsLastMsgSentIndex);
	exit(2);
}

void server(void){
	int sockfd,newfd;
	int opt = 1;
	int i;
	struct sockaddr_in address;
	int addrlen = sizeof(address);
	
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("SERVER:\tsocket failed\n"); 
        exit(EXIT_FAILURE); 
    } 
	
	address.sin_family = AF_INET; 
    address.sin_addr.s_addr = INADDR_ANY; 
    address.sin_port = htons( PORT ); 
	
	// reuse address and port options
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
         &opt, sizeof(opt))) 
        { 
            perror("SERVER:\tsetsockopt\n"); 
            exit(1); 
        } 

    if (bind(sockfd, (struct sockaddr *)&address,  sizeof(address))<0) {
        perror("SERVER:\tbind failed\n"); 
        exit(EXIT_FAILURE); 
    } 
	
	if (listen(sockfd, 3) < 0) {
        perror("SERVER:\tlisten\n"); 
        exit(EXIT_FAILURE); 
    } 

	for (;;){
		printf("SERVER:\tAccepting new connections...\n");
		pthread_mutex_lock(&fd_mutex);
		if ((newfd = accept(sockfd, (struct sockaddr *)&address,  (socklen_t*)&addrlen))<0) {
			perror("accept"); 
			exit(EXIT_FAILURE); 
		} 
		printf("SERVER:\tConnection accepted...\n");
		//printf("qawdqawd\n");
		pthread_t receiveThread;
		pthread_create(&receiveThread, NULL, receiveMsg, &newfd);
		
		i++;
		if (i>=ip_count) i=0;
		
	}
	
}

void *receiveMsg(void *newfd){
	//printf("qawdqawd\n");
	int *temp = (int *) newfd;
	int sock = *temp;

	printf("SERVER:\tWaiting for message\n");
	pthread_mutex_unlock(&fd_mutex);

	char receivedMsg[278];

	recv(sock, receivedMsg, sizeof(receivedMsg), 0);
	printf("SERVER:\tProcessing message:%s\n", receivedMsg);

	int i,doubleMsg,endIndex;
	while (strcmp(receivedMsg,"Exit")!=0){
		doubleMsg=0;
		if (count>=BUFFLENGTH){
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
			pthread_mutex_lock(&buffer_mutex);
			strcpy(buff[count], receivedMsg);
			count++;
			pthread_mutex_unlock(&buffer_mutex);
		}
		send(sock,"OK",strlen("OK")+1,0);
		recv(sock, receivedMsg, sizeof(receivedMsg), 0);
	}
	for (int i=0; i<count;i++){
		printf("\t%s\n",buff[i]);
	}
	printf("Count:%d\n", count);
	printf("SERVER:\tEnding connection...\n");

	close(sock);
	pthread_detach(pthread_self());
	pthread_exit(NULL);
}


void *client(void *param){
    int sock = 0;
    struct sockaddr_in serv_addr; 

    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(PORT); 
    
	int j=0;
	while (j<ip_count){
		printf("CLIENT:\tTrying new connection..\n");
		if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
		{ 
			printf("CLIENT:\tSocket creation error \n"); 
			pthread_exit(NULL); 
		} 
		// Convert IPv4 and IPv6 addresses from text to binary form 
		if(inet_pton(AF_INET, IPs[j], &serv_addr.sin_addr)<=0)  
		{ 
			printf("CLIENT:\tInvalid address/ Address not supported \n"); 
		} 
		
		if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
			printf("CLIENT:\tConnection Failed with IP %s\n", IPs[j]);
		} 
		else {
			sendMsgs(sock, j);			

		}
		close(sock);
		j++;
		if (j == ip_count){
			j = 0;
			sleep(10);
		}
	}
	
	return NULL;
}

void sendMsgs(int sock, int receiver){
	int last_msg_sent_index = IPsLastMsgSentIndex[receiver];
	char *last_msg_sent = IPsLastMsgSent[receiver];

	int endIndex = count;
	if(endIndex >= BUFFLENGTH){
		endIndex = 0;
	}
	char ack[3];

	if((last_msg_sent_index != endIndex) || (strcmp(last_msg_sent, buff[endIndex]) != 0)){
		do {
			last_msg_sent_index++;
			if(last_msg_sent_index >= BUFFLENGTH){
				last_msg_sent_index = 0;
			}

			if (send(sock, buff[last_msg_sent_index], strlen(buff[last_msg_sent_index])+1, 0) == -1){
				break;
			};
			recv(sock, ack,sizeof(ack),0);

			last_msg_sent = buff[last_msg_sent_index];
			
		} while (last_msg_sent_index != endIndex);
		send(sock, "Exit", strlen("Exit") + 1, 0);
		IPsLastMsgSentIndex[receiver] = last_msg_sent_index;
		strcpy(IPsLastMsgSent[receiver], last_msg_sent);
	} else {
		printf("CLIENT:\tNo need of new messages\n");
	}
}

void produceMsg(int sig){
	
	// avoid deadlock on alarm
	if (pthread_mutex_trylock(&buffer_mutex) != 0){
		alarm(10);
		return;
	}
	
	char message[278];
	unsigned int sender = SENDER;
		
	sprintf(
		message, 
		"%d_%d_%d_%s",
		sender, IPsToAEMs[rand() % ip_count], (unsigned)time(NULL), messageList[rand() % message_count]
	);

	if (count >= BUFFLENGTH){
		count = 0;
		fullBuffer = 1;
	}

	strcpy(buff[count++], message);
		
	// alarm(rand() % (5*60 + 1 - 60) + 60);
	alarm(rand() % 10 + 1);
	
	pthread_mutex_unlock(&buffer_mutex);
	
}
