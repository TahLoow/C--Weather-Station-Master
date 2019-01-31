/*
*  engine.c is used to handle the standard input and output of the system
*   -will assert data to be transferred using the shmid
*   -sends top-level requests to operations.c 
*   -creates ALARM signal for incoming requests
*   -sets various timers for errors
*
*  "setAlarmDelay" is used when the signal is received
*  "sig_timeout" is used to put the alarm on timeout while service is provided
*  inside "main()" the shmid is created and data is asserted and prepared for transfer
*
*  Various #include's are used to handle system calls and handle signaling
*   -including other .c files to enable connection throught the entire server
*/

#include <stdio.h>		// for standard system I/O stuff
#include <stdlib.h>	// for exit()
#include <errno.h>		// for error handling on system calls
#include <signal.h>		// for signal handling
#include "network.h"
#include "../project.h"
#include "logger.h"
#include "operations.h"

#define TRUE 1
#define FALSE 0
#define SUCCESS 1
#define FAILURE 0
#define DEBUG 0

#define MAX_RETRIES 3
#define RETRY_FAILURE_DELAY 2
#define RECORDS_UNTIL_ARCHIVE 60 
#define TALK_TO_WS_INTERVAL 60


void timeout();
void delaySeconds();
void sig_timeout(int);
void cancel();
void wsAndLog();

int alrm_cnt = 0;
long num_recorded = 0;
long times_archived = 0;
int dataerror = 0;


int assertWSData(long num_recorded) {
	int attempts = 0;
	int dataerror = setCurrentData(num_recorded); //initialize datasuccessful to false
	while (dataerror && attempts < MAX_RETRIES) {
		logger("assertWSData()","reasserting data...");
		sleep(RETRY_FAILURE_DELAY);
		dataerror = setCurrentData(num_recorded);
		attempts++;
	}
	
	return dataerror;
}

void doDataIteration() {
	signal(SIGALRM, doDataIteration);	// where to go when ALARM signal received
	alarm(TALK_TO_WS_INTERVAL);
	alrm_cnt++;

	logger("main()","Engine iteration **%ld** beginning...",num_recorded);
	printf("\nEngine iteration **%ld** beginning...",num_recorded);

	printf("Setting data...\n");
	dataerror = assertWSData(num_recorded); //assert data. function repeats if data failure
	
	if (!dataerror) {
		printf("Sharing data...\n");
		shareCurrentData();
		
		if (num_recorded%RECORDS_UNTIL_ARCHIVE == 0) {
			printf("Archiving data...\n");
			archiveCurrentData();
			times_archived++;
			
			logger("main()","Archivals: %i",times_archived);
		}
		
		num_recorded++; //update num_recorded
	}
	printf("Iteration complete\n");
	fflush(stdout);
}

void main ()
{
	signal(SIGINT, cancel);	// catch process INTerrupt signals

	closeShm();
	int shmiderror = createShm(); //create one shmid to be used for many transfers
	if (shmiderror) {
		logger("main()","%s","createShmid failed; terminating");
		exit(-1);
	}

	printf("Engine Begun\n");
	logger("main()","%s","Engine started");

	doDataIteration(); //initial iteration @ time 0

		connectNetwork();
}	


void cancel(int signo)
{
	logger("cancel()","\n\n*** Interrupt signal (#%i) caught and ignored ***\n",signo);
    	//closeShm();
	//exit(-1);	
}
