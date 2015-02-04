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
extern int pipeDescr[2];

// http://www.binarytides.com/server-client-example-c-sockets-linux/
// http://www.linuxhowtos.org/C_C++/socket.htm
void *server_client() {
	char buffer[BUFF_SIZE];
	int  result;
	
	printf( "server_client: started\n" );
	while ( ( result = read( pipeDescr[0], &buffer, BUFF_SIZE ) ) > 0 )
 		server_transmit( buffer );
	if ( result == 0 )
		fprintf( stderr, "ERROR in server_client: Pipe closed\n" );
	else
		fprintf( stderr, "ERROR in server_client: Pipe closed with result \n", result );
}

int server_transmit( char * buffer ) {
	int  sockServer, n;
	unsigned int length = sizeof( struct sockaddr_in );
	struct sockaddr_in server, from;
	struct hostent    *serv_host;
	
	// Create socket
	sockServer = socket( AF_INET, SOCK_DGRAM, 0 );
	if ( sockServer == -1 ) {
		fprintf( stderr, "ERROR in server_transmit: Could not create client socket\n" );
		return 1;
	}
	
	serv_host = gethostbyname( configFile.serverAddress );
	if ( serv_host == NULL ) {
		fprintf( stderr, "ERROR in server_transmit: Could not find hostname \"%s\"\n", configFile.serverAddress );
		return 1;
	}
	
	bcopy( (char *)serv_host->h_addr, (char *)&server.sin_addr, serv_host->h_length );
	server.sin_family = AF_INET;
	server.sin_port   = htons( configFile.serverPort );
	
	// Send to remote server
	if ( sendto( sockServer, buffer, strlen( buffer ), 0, (const struct sockaddr *) &server, length ) < 0 ) {
		fprintf( stderr, "ERROR in server_transmit: sendto to server \"%s\" failed\n", buffer );
		return 1;
	}
	
#if _DEBUG > 1
	printf( "Client sent: \"%s\"\n", buffer );
#endif
		
	close( sockServer );
	return 0;
}

// http://www.binarytides.com/server-client-example-c-sockets-linux/
void *server_listen() {
	int sockServer, cs = sizeof( struct sockaddr_in );
	struct sockaddr_in server, client;
	char buffer[BUFF_SIZE];
	
	// Create socket
	sockServer = socket( AF_INET, SOCK_DGRAM, 0 );
	if ( sockServer < 0 ) {
		fprintf( stderr, "ERROR in server_listen: Could not create server socket\n" );
		return;
	}
	
	// Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons( configFile.listenPort );
	
	// Bind
	if ( bind( sockServer,( struct sockaddr *) &server , sizeof( server ) ) < 0 ) {
		fprintf( stderr, "ERROR: Bind failed!\n" );
		return;
	}
	
	while ( 1 ) {
		if ( recvfrom( sockServer, buffer, BUFF_SIZE, 0, (struct sockaddr*) &client, &cs ) < 0 ) {
			fprintf( stderr, "ERROR: recvfrom failed!\n" );
		}
		
#if _DEBUG > 1
		printf( "server_receive: Received \"%s\"\n", buffer );
#endif
  		parse_input( buffer );
	}
}
