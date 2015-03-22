#include "weather-reader.h"

extern ConfigSettings configFile;
SerialDevice *sDev;
int SerDevNum;

/**
 * Main Weather-Reader program function
 */
int main( int argc, char *argv[]) {
	pthread_t threadParser, threadServer;
	struct itimerval timer;
	int    i;
	char **serName;
	
	// Read configuration file
	confReadFile( CONFIG_FILE_NAME, &configFile );
	
	// Open pipes to communicate between uart-server, uart-parser & server-parser
	if ( pipe( pipeParser ) < 0 || pipe( pipeServer ) < 0 ) {
		fprintf( stderr, "ERROR in main: creating server pipe\n" );
		exit(EXIT_FAILURE);
	}
	
#if _DEBUG > 0
	fprintf( stderr, "Debug info:\x1B[30GEnabled\n" );
#endif
	
	// Start client to send data to remote server
	if ( configFile.is_client ) {
		if ( pthread_create( &threadServer, NULL, client_thread, NULL ) < 0) {
			fprintf( stderr, "ERROR in main: creating threadClient\n" );
			exit(EXIT_FAILURE);
		}
	} else {
		// Start parser for local data processing
		if ( pthread_create( &threadParser, NULL, parse_thread, NULL ) < 0 ) {
			fprintf( stderr, "ERROR in main: creating threadParser\n" );
			exit(EXIT_FAILURE);
		}
		// Start server for remote data processing
		if ( configFile.is_server ) {
			if ( pthread_create( &threadServer, NULL, server_thread, NULL ) < 0) {
				fprintf( stderr, "ERROR in main: creating threadServer\n" );
				exit(EXIT_FAILURE);
			}
		}
	}
	
	serName = uart_get_serial();
	for ( SerDevNum = 0; serName[SerDevNum]; SerDevNum++ );
	
	sDev = malloc( sizeof( SerialDevice ) * SerDevNum );
	pthread_t    threadUart[SerDevNum];
	for ( i = 0; serName[i]; i++ ) {
		sDev[i].name   = strdup( serName[i] );
		sDev[i].active = 0;
		sDev[i].tty    = 0;
		sDev[i].no     = i+1;
		sDev[i].type   = 0;
 		free(serName[i]);
		if ( pthread_create( &threadUart[i], NULL, uart_receive, (void *) &sDev[i] ) < 0 ) {
			fprintf( stderr, "ERROR in main: creating threadUart %d\n", (i+1) );
		}
	}
 	free(serName);
	
	// Start timer
	timer.it_value.tv_sec  = 3600;
	timer.it_value.tv_usec = 0;
	timer.it_interval      = timer.it_value;
	if ( setitimer( ITIMER_REAL, &timer, NULL) == -1 )
		fprintf( stderr, "ERROR in main: Could not set timer\n" );

	signal( SIGALRM, (void(*)(int)) signal_alarm );	// Reset Ardiono
	signal( SIGCHLD, SIG_IGN );						// 
	signal( SIGINT, signal_interrupt );				// Program exit
	
	// Wait for threads to complete
// 	pthread_join( threadUart, NULL);
	if ( configFile.is_client ) {
		pthread_join( threadServer, NULL);
	} else if ( configFile.is_server ) {
		pthread_join( threadServer, NULL);
		pthread_join( threadParser, NULL);
	}

#if _DEBUG > 0
	fprintf( stderr, "Program terminated successfully!\n" );
#endif
	exit(EXIT_SUCCESS);
}

/**
 * System signal handler. Takes care of shutting down the program.
 * @param [in] signum The number of the passed interrupt signal
 */
void signal_interrupt( int signum ) {
	configFile.run = 0;
	printf( "\nCaught signal %d\nExiting!\n", signum );
	exit( EXIT_SUCCESS );
}

/**
 * Alarm run every hour. Takes care of resetting any arduinos 
 * and rereading the config file
 */
void signal_alarm( void ) {
	int i;
	for ( i = 0; i < SerDevNum; i++ ) {
		if ( sDev[i].active )
			reset_arduino( &sDev[i] );
	}
	confReadFile( CONFIG_FILE_NAME, &configFile );
}

