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
	int    i, opt, daemon = 0;
	char **serName;
	
    while ( ( opt = getopt( argc, argv, "s" ) ) != -1 ) {
        switch (opt) {
        case 's':
            daemon = 1;
            break;
		default:
			usage();
			break;
		}
	}
	
	if ( daemon == 1 ) {
		pid_t     sid, pid = 0;
		FILE     *file_err;
		
		// Fork the daemon and close main program
		pid = fork(); 		// http://www.thegeekstuff.com/2012/02/c-daemon-process/
		if ( pid < 0 ) {
			fprintf( stderr, "main: Failed to fork!\n" );
			exit( 1 );
		} else if ( pid > 0 ) {
			exit( 0 );
		}
		
		// Redirect STDERR to file
		file_err = fopen( "/var/log/weather-reader.log", "a" );
		if ( file_err ) {
			fflush( stderr );
			dup2( fileno( file_err ), STDERR_FILENO );
			fclose( file_err );
		} else {
			fprintf( stderr, "Error opening /var/log/weather-reader.log\n" );
		}
		
		sid = setsid();
		if ( sid < 0 ) {
			fprintf( stderr, "main: Session id failed!" );
		}
		
		umask( 0 );
		chdir( "/" );
	}
	
	// Read configuration file
	confReadFile( CONFIG_FILE_NAME, &configFile );
	if ( daemon )
		configFile.daemon = 1;

	// Open pipes to communicate between uart-server, uart-parser & server-parser
	if ( pipe( pipeParser ) < 0 || pipe( pipeServer ) < 0 ) {
		fprintf( stderr, "ERROR in %s: creating server pipe\n", __func__ );
		exit(EXIT_FAILURE);
	}
	
#if _DEBUG > 0
	fprintf( stderr, "Debug info: Level %d\n", _DEBUG );
#endif
	
	// Start client to send data to remote server
	if ( configFile.is_client ) {
		if ( pthread_create( &threadServer, NULL, client_thread, NULL ) < 0) {
			fprintf( stderr, "ERROR in %s: creating threadClient\n", __func__ );
			exit(EXIT_FAILURE);
		}
	} else {
		// Start parser for local data processing
		if ( pthread_create( &threadParser, NULL, parse_thread, NULL ) < 0 ) {
			fprintf( stderr, "ERROR in %s: creating threadParser\n", __func__ );
			exit(EXIT_FAILURE);
		}
		// Start server for remote data processing
		if ( configFile.is_server ) {
			if ( pthread_create( &threadServer, NULL, server_thread, NULL ) < 0) {
				fprintf( stderr, "ERROR in %s: creating threadServer\n", __func__ );
				exit(EXIT_FAILURE);
			}
		}
	}
	
	// Scan for USB-serial devices
	serName = uart_get_serial();
	for ( SerDevNum = 0; serName[SerDevNum]; SerDevNum++ );
	
	// Start a thread for each USB-serial device
	sDev = malloc( sizeof( SerialDevice ) * SerDevNum );
	pthread_t threadUart[SerDevNum];
	for ( i = 0; serName[i]; i++ ) {
		sDev[i].name   = strdup( serName[i] );
		sDev[i].active = 0;
		sDev[i].tty    = 0;
		sDev[i].no     = i+1;
		sDev[i].type   = 0;
		sDev[i].head   = 0;
		sDev[i].tail   = 0;
 		free(serName[i]);
		if ( pthread_create( &threadUart[i], NULL, uart_receive, (void *) &sDev[i] ) < 0 ) {
			fprintf( stderr, "ERROR in %s: creating threadUart %d\n", __func__, (i+1) );
		}
	}
 	free(serName);
	
	// Start timer
	timer.it_value.tv_sec  = 3600;
	timer.it_value.tv_usec = 0;
	timer.it_interval      = timer.it_value;
	if ( setitimer( ITIMER_REAL, &timer, NULL) == -1 )
		fprintf( stderr, "ERROR in %s: Could not set timer\n", __func__ );

	signal( SIGALRM, (void(*)(int)) signal_alarm );	// Reset Arduino
	signal( SIGCHLD, SIG_IGN );						// 
	signal( SIGINT, signal_interrupt );				// Program exit
	
	// Wait for threads to complete
	for ( ; SerDevNum > 0; SerDevNum-- )
		pthread_join( threadUart[SerDevNum-1], NULL);
	if ( configFile.is_client ) {
		pthread_join( threadServer, NULL);
	} else if ( configFile.is_server ) {
		pthread_join( threadServer, NULL);
		pthread_join( threadParser, NULL);
	}

	fprintf( stderr, "Program terminated successfully!\n" );
	exit(EXIT_SUCCESS);
}

void usage( void ) {
    fprintf( stderr,
        "weather-reader, a 433 MHz generic data receiver daemon for Weather Stations\n\n"
        "Usage:\t[-s Run program in daemon mode\n\n" );
    exit(1);
}

/**
 * System signal handler. Takes care of shutting down the program.
 * @param [in] signum The number of the passed interrupt signal
 */
void signal_interrupt( int signum ) {
	configFile.run = 0;
	fprintf( stderr, "\nCaught signal %d\n", signum );
	if ( signum == 2 )
		fprintf( stderr, "Program terminated by user!\n" );
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

