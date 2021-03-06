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
	char buffer[BUFF_SIZE], *s;
	int  rcount;
	
	fprintf( stderr, "Client enabled: Using server %s:%d\n", configFile.server, configFile.port );
	
	while ( ( rcount = read( pipeServer[0], &buffer, BUFF_SIZE ) ) > 0 && configFile.run ) {
		buffer[rcount-1] = '\0';
#if _DEBUG > 2
		fprintf( stderr, "client_thread: \"%s\" recv %d bytes\n", buffer, rcount - 1 );
#endif
		// Split multiple inputs
		s = buffer;
		while( s < buffer + rcount ) {
			client_send( s );
			s = strchr( s, 0x0 ) + 1;
		}
	}
	
	if ( rcount == 0 )
		fprintf( stderr, "ERROR in %s: Pipe closed\n", __func__ );
	else
		fprintf( stderr, "ERROR in %s: Pipe error %d\n", __func__, rcount );
	
#if _DEBUG > 0
	fprintf( stderr, "Client thread: Closing\n" );
#endif
}

int client_send( char * buffer ) {
	int  sockServer;
	unsigned int length = sizeof( struct sockaddr_in );
	struct sockaddr_in server;
	struct hostent    *serv_host;
	
	// Create socket
	sockServer = socket( AF_INET, SOCK_DGRAM, 0 );
	if ( sockServer == -1 ) {
		fprintf( stderr, "ERROR in %s: Could not create client socket\n", __func__ );
		return 1;
	}
	
	serv_host = gethostbyname( configFile.server );
	if ( serv_host == NULL ) {
		fprintf( stderr, "ERROR in %s: Could not find hostname \", __func__%s\"\n", configFile.server );
		return 1;
	}
	
	bcopy( (char *)serv_host->h_addr, (char *)&server.sin_addr, serv_host->h_length );
	server.sin_family = AF_INET;
	server.sin_port   = htons( configFile.port );
	
	// Send to remote server
	if ( sendto( sockServer, buffer, strlen( buffer ), 0, (const struct sockaddr *) &server, length ) < 0 ) {
		fprintf( stderr, "ERROR in %s: sendto to server \", __func__%s\" failed\n", buffer );
		return 1;
	}
	
#if _DEBUG > 2
	fprintf( stderr, "client_send: \"%s\" sent %d bytes to %s\n", buffer, strlen( buffer ), inet_ntoa(server.sin_addr) );
#endif
	
	close( sockServer );
	return 0;
}

// http://www.binarytides.com/server-client-example-c-sockets-linux/
void *server_thread() {
	int sockServer, rcount, cs = sizeof( struct sockaddr_in );
	struct sockaddr_in server, client;
	char buffer[BUFF_SIZE];
	
	fprintf( stderr, "Server thread: Started on port %d\n", configFile.port );
		
	// Create socket
	sockServer = socket( AF_INET, SOCK_DGRAM, 0 );
	if ( sockServer < 0 ) {
		fprintf( stderr, "ERROR in %s: Could not create server socket\n", __func__ );
		return;
	}
	
	// Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons( configFile.port );
	
	// Bind
	if ( bind( sockServer,( struct sockaddr *) &server , sizeof( server ) ) < 0 ) {
		fprintf( stderr, "ERROR in %s: Bind failed!\n", __func__ );
		return;
	}
	
	while ( configFile.run ) {
		rcount = recvfrom( sockServer, buffer, BUFF_SIZE, 0, (struct sockaddr*) &client, &cs );
		buffer[rcount++] = '\0';
		if ( rcount < 0 )
			fprintf( stderr, "ERROR in %s: recv from failed!\n", __func__ );

#if _DEBUG > 1
		fprintf( stderr, "server_thread # (%d b): \"%s\" from %s\n", rcount - 1, buffer, inet_ntoa(client.sin_addr) );
#endif
		if ( write( pipeParser[1], &buffer, rcount ) < 1 )
			fprintf( stderr, "ERROR in %s: pipeParser error\n", __func__ );
	}
#if _DEBUG > 0
	fprintf( stderr, "Server thread: Closing\n" );
#endif
}
