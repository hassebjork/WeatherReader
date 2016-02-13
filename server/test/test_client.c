// https://www.abc.se/~m6695/udp.html
#define BUFLEN 512
#define NPACK 10
#define PORT 9876
#define SRV_IP "127.0.0.1"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

void diep( char *s ) {
	perror(s);
	exit(1);
}
  
int main( int argc, char *argv[] ) {
	struct sockaddr_in server;
	int s, i, slen=sizeof( server );
	char buf[BUFLEN];
	
	if ( argc < 2 ) {
		printf( "Usage:\n  %s [ip-address] 'string-to-send'\n", argv[0] );
		return 0;
	}
	
	if ( ( s = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP ) ) == -1 )
		diep( "socket" );

	memset( (char *) &server, 0, sizeof( server ) );
	server.sin_family = AF_INET;
	server.sin_port   = htons( PORT );
	
	if ( inet_aton( argv[1], &server.sin_addr ) == 0 ) {
		fprintf( stderr, "inet_aton() failed\n" );
		exit( 1 );
	}

	sprintf( buf, "%s\0", argv[2] );
	printf( "Sending: %s\n", buf );
	if ( sendto( s, buf, strlen( buf ), 0, (const struct sockaddr *) &server, slen ) ==-1 )
		diep( "sendto()" );

	close( s );
	return 0;
}
