#include <time.h>
#include<arpa/inet.h> //inet_addr
#include<pthread.h> //for threading , link with lpthread
#include<stdio.h>
#include<stdlib.h>    //strlen
#include<string.h>    //strlen
#include<sys/socket.h>
#include<unistd.h>    //write

#define BUFLEN 512
#define PORT 9876

void *connection_handler(void *);
void printTime();

int main(int argc , char *argv[]) {
	struct sockaddr_in server, client;
	int s, rcount, cs = sizeof( client );
	char buf[BUFLEN];
	
	// Create socket
	if ( ( s = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP ) ) ==-1 ) {
		fprintf( stderr, "ERROR: Could not create server socket\n" );
		return 1;
	}
	printTime();
	fprintf( stderr, "\ntest-server version %s (%s)\n", GITVERSION, BUILDTIME );
	puts("\nSocket created");
		
	// Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl( INADDR_ANY );
	server.sin_port = htons( PORT );
	
	// Bind
	if ( bind( s,( struct sockaddr *) &server , sizeof( server ) ) < 0 ) {
		fprintf( stderr, "ERROR: Bind failed!\n" );
		return;
	}
	puts("bind done");
	
	puts("Waiting for incoming connections...");
	while ( 1 ) {
		if ( ( rcount = recvfrom( s, buf, BUFLEN, 0, (struct sockaddr*) &client, &cs ) ) == -1 ) {
			fprintf( stderr, "ERROR: recvfrom failed!\n" );
		}
		buf[rcount++] = '\0';
		
		printTime();	// http://stackoverflow.com/a/23040684
		printf( "Recv from %s:%d \"%s\"\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port), buf );
	}
}
	
void printTime() {
	struct tm *local;
	time_t t = time(NULL);
	local = localtime(&t);
	printf( "\n[%i-%02i-%02i %02i:%02i:%02i]\n", 
			(local->tm_year + 1900), 
			(local->tm_mon) + 1, 
			local->tm_mday, 
			local->tm_hour,
			local->tm_min, 
			local->tm_sec );
}
