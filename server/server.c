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

extern ConfigSettings configFile;

// http://www.binarytides.com/server-client-example-c-sockets-linux/
int create_client( char *str ) {
	int sock;
	struct sockaddr_in server;
	char server_reply[10];
	
	// Create socket
	sock = socket( AF_INET, SOCK_STREAM, 0 );
	if ( sock == -1 )
		fprintf( stderr, "ERROR in create_client: Could not create client socket\n" );
		
	server.sin_addr.s_addr = inet_addr( configFile.serverAddress );
	server.sin_family      = AF_INET;
	server.sin_port        = htons( configFile.serverPort );

	// Connect to remote server
	if ( connect( sock, ( struct sockaddr * ) &server, sizeof( server ) ) < 0 ) {
		fprintf( stderr, "ERROR in create_client: Connection to server failed\n" );
		return 1;
	}
		
	// Receive a reply
	if ( recv( sock, server_reply, 10, 0 ) < 0 ) {
		fprintf( stderr, "ERROR in create_client: recv failed!\n" );
		return 1;
	} else if ( server_reply[0] != 'W' || server_reply[1] != 'R' ) {
		fprintf( stderr, "ERROR in create_client: Wrong server? Reply - \"%s\"!\n", server_reply );
		return 1;
	}
		
	// Send data
	if ( send( sock, str, strlen( str ), 0 ) < 0 ) {
		fprintf( stderr, "ERROR in create_client: Send failed\n" );
		return 1;
	}
#if _DEBUG > 1
	printf( "Client sent: \"%s\"\n", str );
#endif
		
	// Receive a reply
	if ( recv( sock, server_reply, 10, 0 ) < 0 ) {
		fprintf( stderr, "ERROR in create_client: recv failed!\n" );
		return 1;
	} else if ( server_reply[0] != 'O' || server_reply[1] != 'K' ) {
		fprintf( stderr, "ERROR in create_client: Wrong reply - \"%s\"!\n", server_reply );
		return 1;
	}
	
	close( sock );
	return 0;
}

// http://www.binarytides.com/server-client-example-c-sockets-linux/
void *create_server( void *ptr ) {
	pthread_t threadSocket;
	int       sock_desc, sock_client, *sock_new, cs;
	struct sockaddr_in server, client;
	
	// Create socket
	sock_desc = socket( AF_INET, SOCK_STREAM, 0 );
	if ( sock_desc == -1 ) {
		fprintf( stderr, "ERROR in create_server: Could not create server socket\n" );
		return;
	}
	
	// Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons( configFile.listenPort );
	
	// Bind
	if ( bind( sock_desc,( struct sockaddr *) &server , sizeof( server ) ) < 0 ) {
		fprintf( stderr, "ERROR in create_server: Bind failed!\n" );
		return;
	}
	
	// Listen
	listen( sock_desc, 3 );
	
	// Accept incoming connection
	cs = sizeof( struct sockaddr_in );
	while ( ( sock_client = accept( sock_desc, (struct sockaddr *) &client, (socklen_t*) &cs ) ) ) {
		sock_new  = malloc(1);
		*sock_new = sock_client;
#if _DEBUG > 1
		puts( "Connectied" );
#endif
		
		if ( pthread_create( &threadSocket, NULL,  server_receive, (void*) sock_new ) < 0 ) {
			fprintf( stderr, "ERROR in create_server: creating threadSocket\n" );
			free( sock_new );
			return;
		}
	}
		
	if ( sock_client < 0 ) {
		fprintf( stderr, "ERROR in create_server: Connection not accepted\n" );
		free( sock_new );
		return;
	}
	return NULL;
}

void *server_receive( void * socket_desc ) {
	int sock = *(int*)socket_desc;
	int read_size;
	char *send, buffer[512];
		
	//Send ack to the client
	send = "WR\n";
	write( sock, send, strlen( send ) );
	
	//Receive a send from client
	send = "OK\n";
	while( ( read_size = recv( sock, buffer, 512, 0 ) ) > 0 ) {
		buffer[read_size-1] = '\0';
#if _DEBUG > 1
		printf( "server_receive: Received \"%s\"\n", buffer );
#endif
//  		parse_input( buffer );
		write( sock, send, strlen( send ) );
	}
		
	if ( read_size == -1 ) {
		fprintf( stderr, "ERROR in server_receive: recv failed!\n" );
		send = "ER\n";
		write( sock, send, strlen( send ) );
	} else if ( read_size == 0 ) {
#if _DEBUG > 1
		fprintf( stderr, "ERROR in server_receive: Connection closed!\n" );
#endif
	}
	
	free( socket_desc );
	return 0;
}
