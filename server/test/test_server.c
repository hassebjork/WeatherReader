#include <time.h>
#include<arpa/inet.h> //inet_addr
#include<pthread.h> //for threading , link with lpthread
#include<stdio.h>
#include<stdlib.h>    //strlen
#include<string.h>    //strlen
#include<sys/socket.h>
#include<unistd.h>    //write

#define BUFF_SIZE 512

void *connection_handler(void *);
void printTime();

int main(int argc , char *argv[]) {
	int sockServer, cs = sizeof( struct sockaddr_in );
	struct sockaddr_in server, client;
	char buffer[BUFF_SIZE];
	
	// Create socket
	sockServer = socket( AF_INET, SOCK_DGRAM, 0 );
	if ( sockServer < 0 ) {
		fprintf( stderr, "ERROR: Could not create server socket\n" );
		return;
	}
	printTime();
	puts("\nSocket created");
		
	// Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons( 9876 );
	
	// Bind
	if ( bind( sockServer,( struct sockaddr *) &server , sizeof( server ) ) < 0 ) {
		fprintf( stderr, "ERROR: Bind failed!\n" );
		return;
	}
	puts("bind done");
	
	puts("Waiting for incoming connections...");
	while ( 1 ) {
		if ( recvfrom( sockServer, buffer, BUFF_SIZE, 0, (struct sockaddr*) &client, &cs ) < 0 ) {
			fprintf( stderr, "ERROR: recvfrom failed!\n" );
		}
		
		printTime();	// http://stackoverflow.com/a/23040684
		printf( "%s Recv: \"%s\"\n", inet_ntoa(client.sin_addr), buffer );
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
