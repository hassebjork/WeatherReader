/*************************************************************************
   server.c

   This module creates client and server for weather-reader.
   
   Configuration is done in the file weather-reader.conf
   
   This file is part of the Weather Station reader program WeatherReader.
   
   The source code can be found at GitHub 
   https://github.com/hassebjork/WeatherReader
   
**************************************************************************

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS 
   IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED 
   TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A 
   PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT 
   HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
   ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
   OF OR INABILITY TO USE THIS SOFTWARE, EVEN IF THE COPYRIGHT HOLDERS OR 
   CONTRIBUTORS ARE AWARE OF THE POSSIBILITY OF SUCH DAMAGE.

*************************************************************************/

#include "server.h"

#define BUFF_SIZE 512

extern ConfigSettings configFile;

// http://www.binarytides.com/server-client-example-c-sockets-linux/
// http://www.linuxhowtos.org/C_C++/socket.htm
int create_client( char *str ) {
	char buffer[BUFF_SIZE];
	int  sockServer;
	struct sockaddr_in server;
	struct hostent    *serv_host;
	
	// Create socket
	sockServer = socket( AF_INET, SOCK_STREAM, 0 );
	if ( sockServer == -1 ) {
		fprintf( stderr, "ERROR in create_client: Could not create client socket\n" );
		exit(EXIT_FAILURE);
	}
	
	serv_host = gethostbyname( configFile.serverAddress );
	if ( serv_host == NULL ) {
		fprintf( stderr, "ERROR in create_client: Could not find hostname \"%s\"\n", configFile.serverAddress );
		exit(EXIT_FAILURE);
	}
	
	bcopy( (char *)serv_host->h_addr, (char *)&server.sin_addr.s_addr, serv_host->h_length );
	server.sin_family = AF_INET;
	server.sin_port   = htons( configFile.serverPort );

	// Connect to remote server
	if ( connect( sockServer, ( struct sockaddr * ) &server, sizeof( server ) ) < 0 ) {
		fprintf( stderr, "ERROR in create_client: Connection to server \"%s\" failed\n", server.sin_addr.s_addr );
		return 1;
	}
		
	// Receive a reply
	if ( recv( sockServer, buffer, BUFF_SIZE, 0 ) < 0 ) {
		fprintf( stderr, "ERROR in create_client: recv failed!\n" );
		return 1;
	} else if ( buffer[0] != 'W' || buffer[1] != 'R' ) {
		fprintf( stderr, "ERROR in create_client: Wrong server? Reply - \"%s\"!\n", buffer );
		return 1;
	}
		
	// Send data
	if ( send( sockServer, str, strlen( str ), 0 ) < 0 ) {
		fprintf( stderr, "ERROR in create_client: Send failed\n" );
		return 1;
	}
#if _DEBUG > 1
	printf( "Client sent: \"%s\"\n", str );
#endif
		
	// Receive a reply
	if ( recv( sockServer, buffer, BUFF_SIZE, 0 ) < 0 ) {
		fprintf( stderr, "ERROR in create_client: recv failed!\n" );
		return 1;
	} else if ( buffer[0] != 'O' || buffer[1] != 'K' ) {
		fprintf( stderr, "ERROR in create_client: Wrong reply - \"%s\"!\n", buffer );
		return 1;
	}
	
	close( sockServer );
	return 0;
}

// http://www.binarytides.com/server-client-example-c-sockets-linux/
void *create_server( void *ptr ) {
	int sockListen, sockClient, *sockNew, cs;
	pthread_t threadSocket;
	struct sockaddr_in server, client;
	
	// Create socket
	sockListen = socket( AF_INET, SOCK_STREAM, 0 );
	if ( sockListen == -1 ) {
		fprintf( stderr, "ERROR in create_server: Could not create server socket\n" );
		return;
	}
	
	// Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons( configFile.listenPort );
	
	// Bind
	if ( bind( sockListen,( struct sockaddr *) &server , sizeof( server ) ) < 0 ) {
		fprintf( stderr, "ERROR in create_server: Bind failed!\n" );
		return;
	}
	
	// Listen
	listen( sockListen, 3 );
	
	// Accept incoming connection
	cs = sizeof( struct sockaddr_in );
	while ( ( sockClient = accept( sockListen, (struct sockaddr *) &client, (socklen_t*) &cs ) ) ) {
		sockNew  = malloc(1);
		*sockNew = sockClient;
#if _DEBUG > 1
		puts( "Connectied" );
#endif
		
		if ( pthread_create( &threadSocket, NULL,  server_receive, (void*) sockNew ) < 0 ) {
			fprintf( stderr, "ERROR in create_server: creating threadSocket\n" );
			free( sockNew );
			return;
		}
	}
		
	if ( sockClient < 0 ) {
		fprintf( stderr, "ERROR in create_server: Client connection not accepted\n" );
		free( sockNew );
		return;
	}
	return NULL;
}

void *server_receive( void * socket_desc ) {
	int  rcount, sockClient = *(int*) socket_desc;
	char buffer[BUFF_SIZE], *send;
		
	//Send ack to the client
	send = "WR\n";
	write( sockClient, send, strlen( send ) );
	
	//Receive from client
	send = "OK\n";
	while( ( rcount = recv( sockClient, buffer, BUFF_SIZE, 0 ) ) > 0 ) {
		buffer[rcount-1] = '\0';
#if _DEBUG > 1
		printf( "server_receive: Received \"%s\"\n", buffer );
#endif
//  		parse_input( buffer );
		write( sockClient, send, strlen( send ) );
	}
		
	if ( rcount == -1 ) {
		fprintf( stderr, "ERROR in server_receive: recv failed!\n" );
		send = "ER\n";
		write( sockClient, send, strlen( send ) );
	} else if ( rcount == 0 ) {
#if _DEBUG > 1
		fprintf( stderr, "ERROR in server_receive: Connection closed!\n" );
#endif
	}
	
	free( socket_desc );
	return 0;
}
