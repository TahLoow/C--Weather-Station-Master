#ifndef PROJECT_H
#define PROJECT_H

#include <time.h>

/*
		***		MASTER TEAM VALUES. 					***
		***		FEEL FREE TO USE IF YOU WANT THOUGH.	***
		***		I'M NOT THE HEADER FILE POLICE			***
*/
#define MAX_ARCHIVED_RECORDS 1440 //60 records per hour, times 24 hours
#define ERROR -1
#define TRUE 1
#define FALSE 0

//shared memory
#define NETWORKPORT 44001
#define SHMKEY 0x44000
#define NSEC 10

//archive
#define ARCH_DIRECTORY "/home/ftp/pub/cet/440/project/archive/"
#define ARCH_EXT ".dat"

struct wsdatastruct {
	short inside_temp;			//ex: 723 (*0.1 degrees fahrenheit)
	short outside_temp;			//ex: 923 (*0.1 degrees fahrenheit)
	short dewpt;				//ex: 523 (*0.1 degrees fahrenheit)
	short wind_chill;			//ex: -420 (*0.1 degrees fahrenheit)
	short wind_speed;			//ex: 50 (mph)
	short wind_dir;				//ex: 0-359 (degrees from North)
	short barometer;			//ex: 34300 (*0.001 Inches of Mercury)
	short inside_humidity;		//ex: 32 (percent)
	short outside_humidity;		//ex: 80 (percent)
	int total_rain;				//ex: 800 (*0.01 inches of rain)
	short dailyhightemp;
	short dailylowtemp;
	short dailyhighhum;
	short dailylowhum;
	time_t timestamp;			//ex: seconds since midnight (time_t type)
};


#endif

