/*
*  CET440 Weather Station
*  
*  adapted from J. Sumey's "uwho" server  
*
*  Purpose: This is a Unix application program that distributes weather
*           station data to the internet. This program acts as a
*           middleman between a davis weather station computer and
*           clients. Communication between server and client follows a
*           single request server model. The server port is accessible to
*           anybody on the campus network. The weather station data will be
*           delivered to anyone who follows the protocol. Clients
*           query the server by either ASCII strings (current or date):
*           username C
*           username D yyyymmdd
*/ 

#include <stdio.h>  // for standard system I/O stuff 
#include <stdlib.h> 
#include <string.h> 
#include <signal.h>		// for signal handling
#include <sys/types.h>  // for system defined types 
#include <sys/socket.h>  // for socket related definitions 
#include <netinet/in.h>  // for Internet definitions and  conversions 
#include <arpa/inet.h>  // for IP address conversion functions 
#include <netdb.h>  // for network database library def's 
#include <pwd.h>         //for looking up the users
#include "../project.h"
#include "logger.h"
#include "operations.h"
#include <errno.h>

#define  BACKLOG 5 
#define  MAXHOSTNAME 32

connectNetwork() {
	char   localhost[ MAXHOSTNAME+1 ]; // local host name as a string 
	struct hostent *hp;   // result of host name lookup 
	struct sockaddr_in sa, isa;  // Internet socket address struct 
	int  s1, s2;   // local, remote socket descriptors 
	int  i;

	logger("connectNetwork","Initializing socket network");
	// get local host's full name and other info 
	gethostname( localhost, MAXHOSTNAME );
	if ( (hp = gethostbyname( localhost )) == NULL ) { 
		logger("connectNetwork","Hostname error");
		exit(-1);
	}
	// setup socket structure 
	sa.sin_family = AF_INET;  // defined as 2 in socket.h 
	sa.sin_port = htons( NETWORKPORT ); // service port number, supplied from project.h
	memcpy( &sa.sin_addr, hp->h_addr, hp->h_length ); 
	
	// allocate and open local socket 
	if ( (s1 = socket( hp->h_addrtype, SOCK_STREAM, 0 )) < 0 ) {
		logger("connectNetwork","Socket allocation failure");
		exit(-1);
	} 
	
	// bind our socket to our service port 
	if ( bind( s1, (struct sockaddr *) &sa, sizeof sa ) < 0 ) {
		logger("connectNetwork","Bind failure errno: %d. Error: %s\n",errno,strerror(errno));
		exit(-1);
	} 
	
	while(1) {
		// begin listening for connections to our port 
		listen( s1, BACKLOG ); 
		
		// servicing new connections 
		i = sizeof isa;  // because accept() requires &i 
		// call accept() and hang until connection request received 
		if ( (s2 = accept( s1, (struct sockaddr *) &isa, &i )) < 0 ) {
			logger("connectNetwork","Accept error");
			exit(-1); 
		}
		
		// s2 is socket info of remote client
		if(s2 > 0)					// if no error
			processRequest( s2 );   // go look up and respond to request 

		close( s2 );  // close that connection and loop
		logger("connectNetwork","Connection closed");
	}
} /* conneckNetwork*/ 

/* 
*  get request from remote host, format and return a reply
*/ 
processRequest( int sock )
{ 
	struct   passwd *p;          // ptr to password structure 
	char     buf[ BUFSIZ+1 ];    // network I/O buffer
	int      i; 
	struct   sockaddr_in sa;     // Internet socket address struct 
	char	 username[8] = "NO_USER!";
	char     reqcmd = 'k';
	char     datestring[8];      // Parsed request date
	char* 	 client_ip;			

	// get input request from client 
	if ( (i = recv( sock, buf, BUFSIZ, 0 )) <= 0 ) 
		return;    // if error 
	if ( buf[ i-1 ] == '\n' ) i--; 
	if ( buf[ i-1 ] == '\r' ) i--; 
	buf[ i ] = '\0';   // null terminate string 
	
	// get info on other end of connection 
	i = sizeof( struct sockaddr_in ); 
	getpeername( sock, (struct sockaddr *) &sa, &i );
	client_ip = inet_ntoa(sa.sin_addr);
	
	//PARSE CLIENT COMMAND
	//username C OR username D yyyymmdd
	sscanf(buf,"%s %c %s",username,&reqcmd,datestring);
	
	// look up user name in password file -- is it VALID?
	if ( (p = getpwnam( username )) == NULL) {
		logger("processRequest","User not found: %s from IP %s. ",buf,client_ip);
		return -1;
	} else {
		logger("processRequest","User found: %s from IP %s. ",username,client_ip);
	}
	logger("processRequest","Command received: '%c'",reqcmd);
	
	if(tolower(reqcmd)=='c') {
		logger("processRequest","Getting current data...");
		char datastring[100];
		sprintf(datastring,"%i %i %i %i %i %i %i %i %i %i %i %i %i %i %i\n",
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
		send( sock, datastring, strlen(datastring), 0 );
		logger("processRequest","Contents sent");
	} else if(tolower(reqcmd)=='d') {
		logger("processRequest","Getting data from file...");
		
		char tempfilename[255];
		FILE *file;
		sprintf(tempfilename, "%s%s%s",ARCH_DIRECTORY, datestring, ARCH_EXT);
		file = fopen(tempfilename, "r");
		if(file)
		{
			fseek(file, 0, SEEK_END); //stroll to EOF
			int filesize = ftell(file); //get size of file
			char filebuffer[filesize]; //create char array of file size characters
			rewind(file); //daisy on back to the beginning
			fread(filebuffer,sizeof(char),filesize,file); //read x chars
			fclose(file); //close file
			
			send( sock, filebuffer, filesize, 0 );
			logger("processRequest","File contents sent");
		} else {
			logger("processRequest","File does not exist for %s",datestring);
			return -1;
		}
	} else {
		logger("processRequest","Command not valid: %c",reqcmd);
		return -1;
	}
	
	return; 
}
