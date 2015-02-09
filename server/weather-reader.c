#include "weather-reader.h"

extern ConfigSettings configFile;

int main( int argc, char *argv[]) {
	pthread_t threadParser, threadUart, threadServer;
	struct itimerval timer;
	
	confReadFile( CONFIG_FILE_NAME, &configFile );
	
	/* Open pipes */
	if ( pipe( pipeParser ) < 0 || pipe( pipeServer ) < 0 ) {
		fprintf( stderr, "ERROR in main: creating server pipe\n" );
		exit(EXIT_FAILURE);
	}
	
#if _DEBUG > 0
	fprintf( stderr, "Debug info: enabled\n" );
	if ( configFile.sensorReceiveTest > 0 )
		fprintf( stderr, "Sensor receive test: ACTIVE!\n" );
#endif
	
	/* Client thread */
	if ( configFile.is_client ) {
		if ( pthread_create( &threadServer, NULL, client_thread, NULL ) < 0) {
			fprintf( stderr, "ERROR in main: creating threadClient\n" );
			exit(EXIT_FAILURE);
		}
	} else {
		/* Parser thread */
		if ( pthread_create( &threadParser, NULL, parse_thread, NULL ) < 0 ) {
			fprintf( stderr, "ERROR in main: creating threadParser\n" );
			exit(EXIT_FAILURE);
		}
		/* Server thread */
		if ( configFile.is_server ) {
			if ( pthread_create( &threadServer, NULL, server_thread, NULL ) < 0) {
				fprintf( stderr, "ERROR in main: creating threadServer\n" );
				exit(EXIT_FAILURE);
			}
		}
	}

	/* UART thread */
	if ( pthread_create( &threadUart, NULL, uart_receive, NULL ) < 0 ) {
		fprintf( stderr, "ERROR in main: creating threadUart\n" );
		exit(EXIT_FAILURE);
	}
	
	/* Start timer */
	timer.it_value.tv_sec  = 3600;
	timer.it_value.tv_usec = 0;
	timer.it_interval      = timer.it_value;
	if ( setitimer( ITIMER_REAL, &timer, NULL) == -1 )
		fprintf( stderr, "ERROR in main: Could not set timer\n" );

	signal( SIGALRM, (void(*)(int)) signal_alarm );
	signal( SIGCHLD, SIG_IGN );
	signal( SIGINT, signal_interrupt );
	
	/* Wait till threads are complete before main continues.  */
	pthread_join( threadUart, NULL);
	if ( configFile.is_client ) {
		pthread_join( threadServer, NULL);
	} else if ( configFile.is_server ) {
		pthread_join( threadServer, NULL);
		pthread_join( threadParser, NULL);
	}

	exit(EXIT_SUCCESS);
}

void signal_interrupt( int signum ) {
	configFile.run = 0;
	if ( configFile.sensorReceiveTest > 0 ) {
		printf( "\nSaving sensor recieve-test data!\n" );
		sensorSaveTests();
	}
	printf( "Caught signal %d\nExiting!\n", signum );
	exit( EXIT_SUCCESS );
}

void signal_alarm( void ) {
	reset_arduino();
	confReadFile( CONFIG_FILE_NAME, &configFile );
}

