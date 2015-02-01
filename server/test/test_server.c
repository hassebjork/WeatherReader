#include <time.h>
#include<arpa/inet.h> //inet_addr
#include<pthread.h> //for threading , link with lpthread
#include<stdio.h>
#include<stdlib.h>    //strlen
#include<string.h>    //strlen
#include<sys/socket.h>
#include<unistd.h>    //write

void *connection_handler(void *);
void printTime();

int main(int argc , char *argv[]) {
	int socket_desc , client_sock , c , *new_sock;
	struct sockaddr_in server , client;
		
	//Create socket
	socket_desc = socket(AF_INET , SOCK_STREAM , 0);
	if (socket_desc == -1) {
		printf("Could not create socket");
		return 1;
	}
	puts("Socket created");
	
	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons( 9876 );
		
	//Bind
	if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0) {
		perror("bind failed. Error");
		return 1;
	}
	puts("bind done");
		
	//Listen
	listen(socket_desc , 3);
		
	//Accept and incoming connection
	puts("Waiting for incoming connections...");
	c = sizeof(struct sockaddr_in);
	while( (client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) ) {
		printTime();
		puts("\nConnection accepted");
		
		pthread_t sniffer_thread;
		new_sock = malloc(1);
		*new_sock = client_sock;
			
		if( pthread_create( &sniffer_thread , NULL ,  connection_handler , (void*) new_sock) < 0) {
			perror("could not create thread");
			return 1;
		}
			
		//Now join the thread , so that we dont terminate before the thread
		//pthread_join( sniffer_thread , NULL);
	}
		
	if (client_sock < 0) {
		perror("accept failed");
		return 1;
	}
		
	return 0;
}
 
/*
 * This will handle connection for each client
 * */
void *connection_handler(void *socket_desc) {
	//Get the socket descriptor
	int sock = *(int*)socket_desc;
	int read_size;
	char *message , client_message[2000];
		
	//Send some messages to the client
	message = "WR\0";
	write(sock , message , strlen(message));
		
	//Receive a message from client
	while( (read_size = recv(sock , client_message , 2000 , 0)) > 0 ) {
		printf( "Recv: \"%s\"\n", client_message );
	}
		
	if(read_size == 0) {
		puts("Client disconnected\n");
		fflush(stdout);
	} else if(read_size == -1) {
		perror("recv failed");
	}
			
	//Free the socket pointer
	free(socket_desc);
		
	return 0;
}

void printTime() {
	struct tm *local;
	time_t t = time(NULL);
	local = localtime(&t);
	printf( "[%i-%02i-%02i %02i:%02i:%02i]", 
			(local->tm_year + 1900), 
			(local->tm_mon) + 1, 
			local->tm_mday, 
			local->tm_hour,
			local->tm_min, 
			local->tm_sec );
}
