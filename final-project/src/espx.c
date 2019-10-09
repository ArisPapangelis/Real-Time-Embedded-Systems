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
#define SENDERAEM 8883
#define MAX 278
#define BUFFLENGTH 2000
#define MESSAGECOUNT 12
#define LOG_BATCH_SIZE 10
#define STUDENTS 2


void server(void);
void *client(void *); 
void *receiveMsg(void *);
void produceMsg(int);
void sendMsgs(int, int);
void catch_int_term(int);
void logger(char *, int);


char IPs[][11]= { "10.0.88.58", "10.0.89.06" };

unsigned int AEMs[] = {8858, 8906};
char messageList[][256]= {
	"Today is a great day!", "Fancy grabbing a drink later?", "Wanna go to the gym?", "Eat clen tren hard test your limits.",
	"I hate threads.", "This would be done in half the python code!", "Java is the best programming language", "Compound lifts are better than isolation.",
	"Me lo pide sin gorrito.", "Me pongo rolex como si fueran Casio.", "Si tu novio te deja sola.", "Reggaeton is the best musical genre."
};

//Client global variables
int IPsLastMsgSentIndex[STUDENTS];
char IPsLastMsgSent[STUDENTS][278];

//Buffer variables
char buff[BUFFLENGTH][MAX];
int count=0;
int fullBuffer=0;

//Logging variables
char log_buffer[LOG_BATCH_SIZE][MAX];
int curr_log_count = 0;

//Mutexes for thread synchronization
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t fd_mutex = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char *argv[]){
	for (int k=0; k<STUDENTS; k++){
		IPsLastMsgSentIndex[k]=-1;
		strcpy(IPsLastMsgSent[k], "null");
	}


	signal(SIGALRM,produceMsg);
	signal(SIGTERM, catch_int_term);
	signal(SIGINT, catch_int_term);
	srand(time(NULL));


	produceMsg(0);

	pthread_t clientThread;	
	pthread_create(&clientThread, NULL, client, NULL);
	
	server();
	
	return 0;
}

void catch_int_term(int signal){
	printf("INT signal ...\n");
	logger("", 1);
	exit(1);
}

void server(void){
	int sockfd,newfd;
	int opt = 1;
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

		pthread_mutex_lock(&fd_mutex);
		if ((newfd = accept(sockfd, (struct sockaddr *)&address,  (socklen_t*)&addrlen))<0) {
			perror("accept"); 
			exit(EXIT_FAILURE); 
		} 

		pthread_t receiveThread;
		pthread_create(&receiveThread, NULL, receiveMsg, &newfd);
		
		
	}
	
}

void *receiveMsg(void *newfd){
	int *temp = (int *) newfd;
	int sock = *temp;


	pthread_mutex_unlock(&fd_mutex);
	
	char receivedMsg[278];

	recv(sock, receivedMsg, sizeof(receivedMsg), 0);


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
			pthread_mutex_lock(&mutex);
		 	strcpy(buff[count], receivedMsg);
			logger(receivedMsg, 0);
			count++;
			pthread_mutex_unlock(&mutex);
		}

		send(sock,"OK",strlen("OK")+1,0);
		recv(sock, receivedMsg, sizeof(receivedMsg), 0);
	}
	
	//DEBUG
	for (int i=0; i<count;i++){
		printf("\t%s\n",buff[i]);
	}
	printf("Count:%d\n", count);

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
	while (j<STUDENTS){

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
		if (j == STUDENTS){
			j = 0;
			sleep(60);
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

	} 
	else {
		printf("No need of new messages\n");
	}
}

void produceMsg(int sig){
	
	if (pthread_mutex_trylock(&mutex)!=0){
		alarm(3);
		return;
	}
	
	char message[278];
	unsigned int sender = SENDERAEM;
	
	sprintf(message, "%d_%d_%d_%s",sender, AEMs[rand()%STUDENTS], (unsigned)time(NULL), messageList[rand()%MESSAGECOUNT]);
	
	if (count >= BUFFLENGTH){
		count = 0;
		fullBuffer = 1;
	}	
	strcpy(buff[count], message);
	count++;
	logger(message, 0);
	
	int t = rand() % (1*60 + 1 - 20) + 20; //produce message every 20secs to 60secs
	FILE *fp;
	fp = fopen("timebetweenmessages.log", "a+");
	fprintf(fp, "%d\n", t);
	fclose(fp);

	alarm(t);
	
	pthread_mutex_unlock(&mutex);
	
}


void logger(char *receiveMsg, int flush_remaining){

	FILE *fp;
	int i = 0, end = 0;
	strcpy(log_buffer[curr_log_count], receiveMsg);
	curr_log_count++;
	// determine if logging to file will take place
	if (curr_log_count == LOG_BATCH_SIZE || flush_remaining){
		
		end = curr_log_count;
		curr_log_count = 0;
		// write to file
		fp = fopen("statistics.log", "a+");
		for (int i = 0; i < end; i++){
			fprintf(fp, "%s\n", log_buffer[i]);
		}
		fclose(fp);
	}
	return;
}
