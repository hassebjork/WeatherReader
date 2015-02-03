#include "weather-reader.h"

extern ConfigSettings configFile;

int main( int argc, char *argv[]) {
	pthread_t threadUart, threadServer;
	struct itimerval timer;
	
	confReadFile( CONFIG_FILE_NAME, &configFile );
	if ( configFile.is_server ) {
		fprintf( stderr, "Server enabled: using port %d\n", configFile.listenPort );
	} else if ( configFile.is_client ) {
		fprintf( stderr, "Client enabled: using server %s:%d\n", configFile.serverAddress, configFile.serverPort );
	} else {
		sensorInit();
	}
	if ( pipe( pipeDescr ) < 0 ) {
		fprintf( stderr, "ERROR in main: creating client pipe\n" );
		exit(EXIT_FAILURE);
	}
	
#if _DEBUG > 0
	fprintf( stderr, "Debug info: enabled\n" );
	if ( configFile.serverID > 0 ) 
		printf( "This servers ID is %d\n", configFile.serverID );
	if ( configFile.sensorReceiveTest > 0 )
		printf( "Sensor receive test: ACTIVE!\n" );
#endif
	
	/* Server thread */
	if ( configFile.is_server && pthread_create( &threadServer, NULL, server_listen, NULL ) < 0) {
		fprintf( stderr, "ERROR in main: creating threadServer\n" );
		exit(EXIT_FAILURE);
	}
	if ( configFile.is_client ) {
		/* Client thread */
		if ( pthread_create( &threadServer, NULL, server_client, NULL ) < 0) {
			fprintf( stderr, "ERROR in main: creating threadClient\n" );
			exit(EXIT_FAILURE);
		}
	}

	/* UART thread */
	if ( pthread_create( &threadUart, NULL, uart_receive, NULL ) < 0 ) {
		fprintf( stderr, "ERROR in main: creating threadUart\n" );
		exit(EXIT_FAILURE);
	}
	
	/* Start timer */
	signal( SIGINT, signal_interrupt );
	timer.it_value.tv_sec  = 3600;
	timer.it_value.tv_usec = 0;
	timer.it_interval      = timer.it_value;
	if ( setitimer( ITIMER_REAL, &timer, NULL) == -1 )
		fprintf( stderr, "ERROR in main: Could not set timer\n" );

	signal( SIGALRM, (void(*)(int)) signal_alarm );
	
	/* Wait till threads are complete before main continues.  */
	pthread_join( threadUart, NULL);

	exit(EXIT_SUCCESS);
}

void signal_interrupt( int signum ) {
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

