/*
*  logger.c utilizes the function "void logger()" found in logger.h
*  logger.c will output data to error file instead of console
*  	 Takes in a character array for name of function and arguments
*	 Will format into the outout message
*
*  "struct tm* openFile()" will open the file and utilize the exact time
*  "void logger()" will set up the format
*/

#include <stdio.h>		// for standard system I/O stuff
#define LOG_DIR "/home/ftp/pub/cet/440/project/2018/master/Logs/"
#define LOG_EXT ".txt"
#include <errno.h>
#include <stdarg.h>
#include "string.h"
#include "time.h"

#define DEBUG 0

FILE *errorfile;

struct tm* openFile() {
	char filename[255];
	
	struct tm *tm;
	time_t clock;       
	time(&clock);                   // GMT time of day from system\
	
	int year,mon,day;
	tm=localtime(&clock);           // get local time
	year = (tm->tm_year) + 1900;
	mon	= (tm->tm_mon) + 1;
	day = (tm->tm_mday);
	
	sprintf(filename, "%s%4d%02d%02d%s",LOG_DIR, year, mon, day, LOG_EXT);
	errorfile = fopen(filename,"a+");
	
	return tm;
}

//logger will output to error file instead of console
void logger(char location[300],char format[300],...) {
	char outputmsg[300];
	
	int hour,min,sec;
	struct tm *tm = openFile();
	
	hour = (tm->tm_hour);
	min = (tm->tm_min);
	sec = (tm->tm_sec);
	
	
	//printf("[ %s ]:\n",location);
	
	va_list argptr;
    va_start(argptr, format);
    vsprintf(outputmsg,format, argptr);
    va_end(argptr);
    
    //printf("outputmsg: %s\n",outputmsg);

	fprintf(errorfile,"%02d:%02d:%02d: [ %s ]: %s\n",hour,min,sec,location,outputmsg);
	if (errno > 0 && errno != 4)
		fprintf(errorfile,"\t\t\t***errno: %d : || %s***\n",errno,strerror(errno));
	
	fclose(errorfile);
	fflush(stdout);
}

