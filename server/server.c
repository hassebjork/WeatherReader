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
extern int pipeServer[2];
extern int pipeParser[2];

// http://www.binarytides.com/server-client-example-c-sockets-linux/
// http://www.linuxhowtos.org/C_C++/socket.htm
void *client_thread() {
	char buffer[BUFF_SIZE];
	int  result;
	
	fprintf( stderr, "Client enabled:\x1B[30GUsing server %s:%d\n", configFile.server, configFile.port );
	
	while ( ( result = read( pipeServer[0], &buffer, BUFF_SIZE ) ) > 0 && configFile.run )
 		client_send( buffer );
	if ( result == 0 )
		fprintf( stderr, "ERROR in client_thread: Pipe closed\n" );
	else
		fprintf( stderr, "ERROR in client_thread: Pipe error %d\n", result );
	
#if _DEBUG > 1
	fprintf( stderr, "Client thread:\x1B[30GClosing\n" );
#endif
}

int client_send( char * buffer ) {
	int  sockServer, n;
	unsigned int length = sizeof( struct sockaddr_in );
	struct sockaddr_in server, from;
	struct hostent    *serv_host;
	
	// Create socket
	sockServer = socket( AF_INET, SOCK_DGRAM, 0 );
	if ( sockServer == -1 ) {
		fprintf( stderr, "ERROR in client_send: Could not create client socket\n" );
		return 1;
	}
	
	serv_host = gethostbyname( configFile.server );
	if ( serv_host == NULL ) {
		fprintf( stderr, "ERROR in client_send: Could not find hostname \"%s\"\n", configFile.server );
		return 1;
	}
	
	bcopy( (char *)serv_host->h_addr, (char *)&server.sin_addr, serv_host->h_length );
	server.sin_family = AF_INET;
	server.sin_port   = htons( configFile.port );
	
	// Send to remote server
	if ( sendto( sockServer, buffer, strlen( buffer ), 0, (const struct sockaddr *) &server, length ) < 0 ) {
		fprintf( stderr, "ERROR in client_send: sendto to server \"%s\" failed\n", buffer );
		return 1;
	}
	
#if _DEBUG > 1
	printf( "client_send:\x1B[30G\"%s\" %s\n", buffer, inet_ntoa(server.sin_addr) );
#endif
	
	close( sockServer );
	return 0;
}

// http://www.binarytides.com/server-client-example-c-sockets-linux/
void *server_thread() {
	int sockServer, rcount, cs = sizeof( struct sockaddr_in );
	struct sockaddr_in server, client;
	char buffer[BUFF_SIZE];
	
	fprintf( stderr, "Server enabled:\x1B[30GListening on port %d\n", configFile.port );
		
	// Create socket
	sockServer = socket( AF_INET, SOCK_DGRAM, 0 );
	if ( sockServer < 0 ) {
		fprintf( stderr, "ERROR in server_thread: Could not create server socket\n" );
		return;
	}
	
	// Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons( configFile.port );
	
	// Bind
	if ( bind( sockServer,( struct sockaddr *) &server , sizeof( server ) ) < 0 ) {
		fprintf( stderr, "ERROR in server_thread: Bind failed!\n" );
		return;
	}
	
	while ( configFile.run ) {
		rcount = recvfrom( sockServer, buffer, BUFF_SIZE, 0, (struct sockaddr*) &client, &cs );
		buffer[rcount++] = '\0';
		if ( rcount < 0 )
			fprintf( stderr, "ERROR in server_thread: recvfrom failed!\n" );

#if _DEBUG > 1
		printf( "server_thread:\x1B[30G\"%s\" %s\n", buffer, inet_ntoa(client.sin_addr) );
#endif
		if ( write( pipeParser[1], &buffer, rcount ) < 1 )
			fprintf( stderr, "ERROR in server_thread: pipeParser error\n" );
	}
#if _DEBUG > 1
	fprintf( stderr, "Server thread:\x1B[30GClosing\n" );
#endif
}
