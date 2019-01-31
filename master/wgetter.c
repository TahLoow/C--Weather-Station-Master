/*
 *  wgetter.c is derived from w2.c and utilizes wgetter.h
 *  last updated on 11/15/2018
 *  -retrieves raw data from the weather station and categorizes each piece into the correct category
 *		-temperature
 *		-wind speed and direction 
 *		-humidity and dewpoint
 *		-barometer, total rain, etc.
 * 	-converts necessary data pieces into appropriate readings
 *	Uses "logger" function to format data   
 */
 
#include <stdio.h>
#include <errno.h>	// for error handling on system calls
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <termio.h>	// for manipulating termio line settings
#include "ccitt.h"	// CCITT CRC computation data table
#include "logger.h"
#include "../project.h"

/* set desired debugging level between 0 (none) and 3 (most) */
#define DEBUG 0

/* define device file Weatherlink is interfaced to */
#define DEV "/dev/ttyS1"

/* common ASCII constants */
#define CR  0x0D
#define ACK 0x06
#define NAK 0x15

int WSfd;		// global Weather Station file descriptor

//prototypes
int getRawData();
void interpretRawData(struct wsdatastruct*);

/* define sensor image memory layout */
/* note: this assumes shorts are LSB first (Intel style) */
#pragma pack(1)		// force following structure to 18 contiguous bytes
struct si_struct 
{
    u_char start_block;
    short inside_temp;
    short outside_temp;
    u_char wind_speed;
    short wind_dir;
    short barometer;
    u_char inside_humidity;
    u_char outside_humidity;
    ushort total_rain;
    ushort fill;        // not used
    ushort crc;
} sensor_image;         // declares global structure variable
#pragma pack()		// reset default compiler setting

short dewpoint;
short windchill;

//============
//				more behind-the-scenes functions
//============

int ws_open(char *devname) { //opens file to Weather Link interface & return file descriptor
    struct termio TermInfo;

    // open tty line to Weather Station as read-write, exclusive access
    if ((WSfd = open(devname, O_RDWR | O_EXCL)) == -1) {
    	logger("ws_open","opening failed");
    	return WSfd;
	}
    
    // now setup required line settings
    memset(&TermInfo, 0, sizeof( TermInfo));
    TermInfo.c_iflag = 0;		// input modes
    TermInfo.c_oflag = 0;		// output modes
    TermInfo.c_cflag = CLOCAL | CREAD | CS8 | B2400;    // control modes
    TermInfo.c_lflag = 0;			// local modes
    TermInfo.c_cc[VMIN] = 1;		// must read at least 1 character
    ioctl(WSfd, TCFLSH, 2);			// flush both input & output in case
    ioctl(WSfd, TCSETA, &TermInfo);	// establish new line settings
    return WSfd;			// return file descriptor number
}

int ws_close()
{
    return (close(WSfd));
} // ws_close()

int ws_getc() { //retrieves & returns one character from the Weather Link interface
    u_char c = 0;
    if (read(WSfd, &c, 1) != 1) {
    	logger("ws_getc()","getc error");
    	return -1;
	}
    return (int)c;
}

int ws_gets(char *s, int n) {
  // s: pointer to where received chars. to be stored
  // n: number of chars. to get & store
	int c;
	
	do {
	    if ((c = ws_getc()) == -1) {
	    	logger("ws_getc()","getc error");
	        return -1;
		}
	    *s++ = (char)c;
	} while (--n > 0);
	return 0;
}

int ws_putc(u_char c)
{
    if (write(WSfd, &c, 1) != 1) {
    	logger("ws_putc()","putc error");
        return -1;
    }
    return 0;
}

int ws_puts(char *s)
{
    for ( ; *s != '\0'; s++)
        if (ws_putc(*s) != 0)
            return -1;
    return 0;
}


int getFormattedData(struct wsdatastruct* currentData) {
	int dataerror = getRawData();
	
	if (dataerror) {
		logger("getFormattedData()","Recognized data error");
		return -1;
	};
	
	interpretRawData(currentData); //update currentData with img values, dew point & wind chill
	
#if DEBUG > 1
	printf("\n\nOutdoor Temperature is %.1f Degrees\n",(float)sensor_image.outside_temp/10);
	printf("Indoor Temperature is %.1f Degrees\n",(float)sensor_image.inside_temp/10);
	printf("Rain is %.2f Inches\n", (float)sensor_image.total_rain/100);
	if (sensor_image.wind_speed == 0)
		printf("Winds are calm\n");
	else {
		printf("Wind Direction is %i degrees\n", sensor_image.wind_dir);
		printf("Wind Speed is %i MPH\n", sensor_image.wind_speed);
	}
	printf("Outside Humidity is %i%%\n", sensor_image.outside_humidity);
	printf("Inside Humidity is %i%%\n", sensor_image.inside_humidity);
	printf("Barometer is %.3f Inches of Mercury\n\n", (float)sensor_image.barometer/1000);
	
	printf("Wind chill: %i\n",windchill);
	printf("Dew point: %i\n",dewpoint);
#endif
	
	return dataerror;
}

void interpretRawData(struct wsdatastruct* currentData) {
	currentData->barometer= sensor_image.barometer;
	currentData->inside_humidity = (short)sensor_image.inside_humidity;
	currentData->outside_humidity= (short)sensor_image.outside_humidity;
	currentData->inside_temp = sensor_image.inside_temp;
	currentData->outside_temp = sensor_image.outside_temp;
	currentData->total_rain  = (int)sensor_image.total_rain;
	currentData->wind_dir = sensor_image.wind_dir;
	currentData->wind_speed = (short)sensor_image.wind_speed;
	
	currentData->dewpt = dewpoint;
	currentData->wind_chill = windchill;
}

int getRawData() {
	int c;
	
    // open line to weather station
    if ( (WSfd = ws_open(DEV)) == -1 ) {
        logger("getRawData()","failed to open ws");
        return -1;
    }
    //Send IMaGe command to force a sensor reading
    ws_puts("IMG");
    ws_putc(CR);
    
    if ((c = ws_getc()) != ACK) {
    	logger("getRawData()","IMG ACK failed");
    	return -1;
	}
	
    //Send LOOP command to obtain sensor readings
    ws_puts("LOOP");
    ws_putc(0xFF);    // hi byte of two's complement of n=1
    ws_putc(0xFF);    // lo byte of two's complement of n=1
    ws_putc(CR);
    
    if ((c = ws_getc()) != ACK) {
		logger("getRawData()","LOOP ACK failed");
		return -1;
	} else
        ws_gets((char *)&sensor_image, sizeof(sensor_image));	//read in sensor image
    
    
    
    
    //Image data captured & stored
    //Now capture and store windchill & dewpoint
    
    { //Capture windchill
		ws_puts("WRD");			// Send the WRD command.
		ws_putc(0x42);			// 4 nibbles, bank 0 (represented by 2; see wlprotocols.txt)
		ws_putc(0xA8);			// Address for wind chill
		ws_putc(CR);			// Send CR...
	    
	    if ((c = ws_getc()) != ACK) {
			logger("getRawData()","WRD ACK failed - Windchill");
			return -1;
		} else
			ws_gets((char *)&windchill,sizeof(windchill));		//read in windchill
	}
	
	
	{ //Capture dewpoint
		ws_puts("WRD");			// Send the WRD command.
		ws_putc(0x42);			// 4 nibbles, bank 0 (represented by 2; see wlprotocols.txt)
		ws_putc(0x8A);			// Address for dew point
		ws_putc(CR);			// Send CR
	    
	    if ((c = ws_getc()) != ACK) {
			logger("getRawData()","WRD ACK failed - Dewpoint");
			return -1;
		}
		else
			ws_gets((char *)&dewpoint,sizeof(dewpoint));	//read in dewpoint
	}
	
	if (ws_close()) {
		logger("getRawData()","failed to close ws");
		return -1;
	}
	
    return 0;
}
