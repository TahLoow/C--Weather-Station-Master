/*
*  operations.c utilizes "struct wsdatastruct currentData;" found in operations.h
*  - handles top-level requests from engine.c, such as:
*	 -assertWSData()
*	 -setCurrentData()
*	 -shareCurrentData()
*
*  Various #includes are called to handle errors, shared memory facilities, and manipulate settings
*	 -also we #include other header files to formally bring all files to life
*			-"logger.h"
*			-"wgetter.h"
*			-"operations.h"
*			-"../project.h"
*  "struct wsdatastruct" will constantly be overwritten
*/

#include <stdio.h>
#include <errno.h>	// for error handling on system calls
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h> // for shared memory facility defines
#include <fcntl.h>
#include <string.h>
#include <termio.h>	// for manipulating termio line settings
#include <time.h>
#include <signal.h>

#include "logger.h"
#include "wgetter.h"
#include "operations.h"
#include "../project.h"

#define DEBUG 0

//struct wsdatatruct currentData defined in project.h
struct wsdatastruct localArchive[MAX_ARCHIVED_RECORDS];


//shared memory global variables
struct wsdatastruct *sharedCurrentData;
int shm_id;
long shm_addr;

//file info for public archive
FILE* outfile;
char lastfilename[255];


time_t getLocalTime() {
	time_t clock;                	// struct for time function
	struct tm *tm;					// making an object        
	time(&clock);                   // GMT time of day from system
	
	return clock;           // get local time
}

short max(short num1, short num2) 
{
	if (num1 > num2)
	 return num1;
	else 
	 return num2;
}

short min(short num1, short num2)
{
	if (num1<num2)
	 return num1;
	else 
	 return num2;
}

void getDaily(long num_recorded)
{
	printf("Getting Dailies...\n");
	short highTemp = -5000;
	short lowTemp = 5000;
	short highHumidity = -5000;
	short lowHumidity = 5000;
	int i;
	
	long upto = num_recorded;
	
	if (num_recorded >= MAX_ARCHIVED_RECORDS)
		upto = MAX_ARCHIVED_RECORDS-1;
	
	
	for(i = 0; i<=upto; i++)
	{
		highTemp = max(localArchive[i].outside_temp,highTemp);
		lowTemp = min(localArchive[i].outside_temp,lowTemp);
		highHumidity = max(localArchive[i].outside_humidity, highHumidity);
		lowHumidity = min(localArchive[i].outside_humidity, lowHumidity);
		
	}
	
	currentData.dailyhightemp = highTemp;
	currentData.dailylowtemp = lowTemp;
	currentData.dailyhighhum = highHumidity;
	currentData.dailylowhum = lowHumidity;
	printf("Dailies Got\n");
}

//shared memories
int setShmId() {
	shm_id = shmget(SHMKEY, sizeof(struct wsdatastruct), IPC_CREAT | IPC_EXCL | 0644);
	if (shm_id == -1) {
    	logger("shmget()","%s","Get failed");
		return -1;
	}
	return 0;
}

int createShm () {
    sharedCurrentData = &currentData;
	
	setShmId();
	
	sharedCurrentData = (struct wsdatastruct*)shmat(shm_id, NULL, 0);
	if (sharedCurrentData == (void*) -1) {
		logger("shmat()","%s","Attachment failed");
		return -1;
	}
	return 0;
}

int closeShm() {
	printf("Getting existing shm id...\n");
	int existing_id = shmget(SHMKEY, sizeof(struct wsdatastruct), 0644);
	
	//shmdt(sharedCurrentData);
	int ctlerror = shmctl(existing_id, IPC_RMID, NULL);
    if (ctlerror) {
		logger("closeShm()","shmdt failed to detach non-attached shmid");
		return -1;
	}
}


//=============
//				high-level core .h functions below
//=============

//updates global currentData struct with correct information
int setCurrentData(long num_recorded) {
	int dataerror = getFormattedData(&currentData);
	if (dataerror)
		return -1;
	
	currentData.timestamp = getLocalTime();
	
	localArchive[num_recorded%1440] = currentData;
	getDaily(num_recorded);

	//currentdata now holds most recent weather stuff, timestamp, and highs/lows
	return 0;
}


//shareCurrentData() to write to shared memory:
int shareCurrentData() {
	*sharedCurrentData = currentData;
    return 0;
}

//formats currentData into an archive file. File name dependent on date.
void archiveCurrentData() {
	char tempfilename[255];
	struct tm *tm;									// making an object
	
	int year,mon,day;
	tm=localtime(&currentData.timestamp);           // get local time
	year = (tm->tm_year) + 1900;
	mon	= (tm->tm_mon) + 1;
	day = (tm->tm_mday);
	
	//put proper format of filename into tempfilename
	sprintf(tempfilename, "%s%4d%02d%02d%s",ARCH_DIRECTORY, year, mon, day, ARCH_EXT);
	
	//check to see if the last filename is the same as the new filename. log the result.
	if (strncmp(tempfilename,lastfilename,sizeof(tempfilename)) == 0)
		logger("archiveCurrentData()","Updating archive file");
	else
		logger("archiveCurrentData()","Creating new archive file");
	
	sprintf(lastfilename,tempfilename);
	outfile = fopen(lastfilename,"a+");
	
	fprintf(outfile,"%i %i %i %i %i %i %i %i %i %i %i %i %i %i %i\n",
		currentData.timestamp,
		currentData.inside_temp,
		currentData.outside_temp,
		currentData.dewpt,
		currentData.wind_chill,
		currentData.wind_speed,
		currentData.wind_dir,
		currentData.barometer,
		currentData.inside_humidity,
		currentData.outside_humidity,
		currentData.total_rain,
		currentData.dailyhightemp,
		currentData.dailylowtemp,//
		currentData.dailyhighhum,
		currentData.dailylowhum
	);
	
	fclose(outfile);
	fflush(stdout);
}

