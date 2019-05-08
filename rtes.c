#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>

void samplingWithoutTimeStamps (int, float);

void samplingWithTimeStamps (int, float);

void handleAlarm (int);

int flag=1;

int main(int argc, char *argv[]){
	int t=7200;
	float dt= 0.1;
	
	
	signal(SIGALRM,handleAlarm);
	
	samplingWithoutTimeStamps(t,dt);
	
	flag=1;
	samplingWithTimeStamps(t,dt);

	
}


void samplingWithoutTimeStamps (int t, float dt){
	
	int size = (int)((float)t / dt);
	
	struct timeval timeStamp;
	double timeStamps[size];
	
	struct timespec sleepTime, timeRemaining;
	sleepTime.tv_sec = 0;
	sleepTime.tv_nsec = (long int) (1000000000*dt); 		//100 ms
	
	//Getting timestamps for specified time.
	int i=0;
	alarm(t);
	while (flag){
		gettimeofday(&timeStamp,NULL);
		timeStamps[i] = timeStamp.tv_sec + 0.000001*timeStamp.tv_usec;
		nanosleep(&sleepTime, &timeRemaining);
		i++;
	}
	
	/*Printing results to console.
	for (int i=0; i<sizeof(timeStamps)/sizeof(timeStamps[0]); i++){
		printf("%d. Timestamp: %lf	\n", i+1, timeStamps[i]-timeStamps[0]);
	}
	*/
	
	//Writing results to file.
	FILE *fp;
	fp=fopen("withoutTimestamps.txt","w");
	for (i=0;i<size+10;i++){
		fprintf(fp,"%lf\n",timeStamps[i]);
	}
	fclose(fp);

}

void samplingWithTimeStamps (int t,float dt){
		
	int size = (int)((float)t / dt);
	
	struct timeval timeStamp;
	double timeStamps[size+10];

	struct timespec sleepTime, timeRemaining;
	sleepTime.tv_sec = 0;
	sleepTime.tv_nsec = (long int) (1000000000*dt); 		//100 ms
	
	//Getting timestamps for specified time.
	int i=0;
	alarm(t);
	gettimeofday(&timeStamp,NULL);
	timeStamps[i++] = timeStamp.tv_sec + 0.000001*timeStamp.tv_usec;
	nanosleep(&sleepTime, &timeRemaining);
	while (flag){
		gettimeofday(&timeStamp,NULL);
		timeStamps[i] = timeStamp.tv_sec + 0.000001*timeStamp.tv_usec;
		sleepTime.tv_nsec=(long int)(1000000000*dt-(timeStamps[i]-timeStamps[0]-dt*i)*1000000000);
		nanosleep(&sleepTime, &timeRemaining);
		i++;
	}
	
	/*Printing results to console.
	for (int i=0; i<sizeof(timeStamps)/sizeof(timeStamps[0]); i++){
		printf("%d. Timestamp: %lf	\n", i+1, timeStamps[i]-timeStamps[0]);
	}
	*/
	
	//Writing results to file.
	FILE *fp;
	fp=fopen("withTimestamps.txt","w");
	for (i=0;i<size+10;i++){
		fprintf(fp,"%lf\n",timeStamps[i]);
	}
	fclose(fp);
	
}

void handleAlarm(int sig){
	flag=0;
}
	