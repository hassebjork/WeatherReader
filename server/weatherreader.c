#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <signal.h>
#include "weatherreader.h"

int terminate = 0;

void signal_callback_handler( int signum ) {
	printf( "\nCaught signal %d\nExiting!\n", signum );
	terminate = 1;
	exit( EXIT_SUCCESS );
}

// http://www.yolinux.com/TUTORIALS/LinuxTutorialPosixThreads.html
void *uart_receive( void *ptr ) {
	struct termios options;
	int    fd, rcount;
	char   buffer[256];
	char  *dev;
	dev = (char *) ptr;

	fd = open( dev, O_RDWR ); // | O_NOCTTY | O_NDELAY );
	if ( fd == -1 ) {
		perror( dev );
		exit(EXIT_FAILURE);
	}
	printf( "Opening %s\n", dev );
	
	if ( tcgetattr( fd, &options ) < 0 ) {
		perror( "ERROR: Getting configuration" );
		close( fd );
		exit(EXIT_FAILURE);
	}
	
	options.c_cflag = B115200 | CS8 | CREAD | CLOCAL; // 8 data bits, enable receiver, local line 
	options.c_cflag &= ~PARENB | ~CSTOPB; // No parity, one stop bit 
	options.c_iflag &= ~( IXON | IXOFF | IXANY ); // Disable software flow control
	options.c_oflag = 0;
	options.c_lflag = ICANON; // Read line
	options.c_cc[VTIME] = 0; // No timeout clock for read
	options.c_cc[VMIN] = 1; // Wait for one character before reading another
	tcflush( fd, TCIFLUSH ); 
	tcsetattr( fd, TCSANOW, &options ); // Set fd with new settings immediately
	
	while ( !terminate ) {
		rcount = read( fd, buffer, sizeof( buffer ) );
		if ( rcount < 0 ) {
			perror( "Read" );
		} else if ( rcount > 0 ) {
			buffer[rcount-1] = '\0';
			printf( "%s\n", buffer );
		}
		// http://stackoverflow.com/questions/12777254/time-delay-in-c-usleep
		usleep( 200 );
	}
	
	close( fd );
}

int main( int argc, char *argv[]) {
	pthread_t thread1;
	const char *dev0 = "/dev/ttyUSB0";
	int iret1;
	
	/* Create independent threads each of which will execute function */
	iret1 = pthread_create( &thread1, NULL, uart_receive, (void*) dev0 );
	if ( iret1 ) {
		fprintf( stderr, "Error - pthread_create() return code: %d\n", iret1 );
		exit(EXIT_FAILURE);
	}

	signal( SIGINT, signal_callback_handler );

	/* Wait till threads are complete before main continues.  */
	pthread_join( thread1, NULL);

	exit(EXIT_SUCCESS);
}
