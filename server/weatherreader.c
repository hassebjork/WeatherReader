#include "weatherreader.h"

int tty, terminate = 0;
extern ConfigSettings configFile;

static unsigned char reverse_bits_lookup[16] = {
	0x0, 0x8, 0x4, 0xC, 0x2, 0xA, 0x6, 0xE,
	0x1, 0x9, 0x5, 0xD, 0x3, 0xB, 0x7, 0xF
};

int main( int argc, char *argv[]) {
	pthread_t threadUart, threadServer;
	struct itimerval timer;
	
	confReadFile( CONFIG_FILE_NAME, &configFile );
	sensorInit();
#if _DEBUG > 0
	char s[20];
	printTime( s );
	fprintf( stderr, "%s Debug mode enabled\n", s );
	if ( configFile.serverID > 0 ) 
		printf( "This servers ID is %d\n", configFile.serverID );
	if ( configFile.sensorReceiveTest > 0 )
		printf( "Sensor receive test is ACTIVE!\n" );
	
#endif
	
	/* Create independent threads each of which will execute function */
	if ( pthread_create( &threadUart, NULL, uart_receive, NULL ) < 0 ) {
		fprintf( stderr, "ERROR in main: creating threadUart\n" );
		exit(EXIT_FAILURE);
	}
	
	if ( configFile.is_server && pthread_create( &threadServer, NULL, create_server, NULL ) < 0) {
		fprintf( stderr, "ERROR in main: creating threadServer\n" );
		exit(EXIT_FAILURE);
	}

	signal( SIGINT, signal_interrupt );
	timer.it_value.tv_sec  = 3600;
	timer.it_value.tv_usec = 0;
	timer.it_interval      = timer.it_value;
	if ( setitimer( ITIMER_REAL, &timer, NULL) == -1 )
		fprintf( stderr, "ERROR in main: Could not set timer\n" );

	/* Wait till threads are complete before main continues.  */
	pthread_join( threadUart, NULL);

	exit(EXIT_SUCCESS);
}

void signal_interrupt( int signum ) {
	terminate = 1;
	if ( configFile.sensorReceiveTest > 0 ) {
		printf( "\nSaving sensor recieve-test data!\n", signum );
		sensorSaveTests();
	}
	printf( "Caught signal %d\nExiting!\n", signum );
	exit( EXIT_SUCCESS );
}

void signal_alarm( void ) {
	reset_arduino();
	confReadFile( CONFIG_FILE_NAME, &configFile );
}

void reset_arduino( void ) {
	// http://www.linuxtv.org/mailinglists/vdr/2003/02-2003/msg00543.html
	int flag = TIOCM_DTR;
	ioctl( tty, TIOCMBIC, &flag );
	sleep( 1 );
	ioctl( tty, TIOCMBIS, &flag );
#if _DEBUG > 1
	char s[20];
	printTime( s );
	fprintf( stderr, "%s Resetting Arduino\n", s );
#endif
}

// http://www.binarytides.com/server-client-example-c-sockets-linux/
void *create_server( void *ptr ) {
	pthread_t threadSocket;
	int       sock_desc, sock_client, *sock_new, cs;
	struct sockaddr_in server, client;
	
	//Create socket
	sock_desc = socket( AF_INET, SOCK_STREAM, 0 );
	if ( sock_desc == -1 ) {
		fprintf( stderr, "ERROR in create_server: Could not create server socket\n" );
		return;
	}
	
	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons( configFile.listenPort );
	
	//Bind
	if ( bind( sock_desc,( struct sockaddr *) &server , sizeof( server ) ) < 0 ) {
		fprintf( stderr, "ERROR in create_server: Bind failed!\n" );
		return;
	}
	
	//Listen
	listen( sock_desc, 3 );
	
	//Accept and incoming connection
	cs = sizeof( struct sockaddr_in );
	while ( ( sock_client = accept( sock_desc, (struct sockaddr *) &client, (socklen_t*) &cs ) ) ) {
		sock_new  = malloc(1);
		*sock_new = sock_client;
		
		if ( pthread_create( &threadSocket, NULL,  server_receive, (void*) sock_new ) < 0 ) {
			fprintf( stderr, "ERROR in create_server: creating threadSocket\n" );
			return;
		}
	}
		
	if ( sock_client < 0 ) {
		fprintf( stderr, "ERROR in create_server: Connection not accepted\n" );
		return;
	}
	return NULL;
}

void *server_receive( void * socket_desc ) {
	int sock = *(int*)socket_desc;
	int read_size;
	char *send, buffer[512];
		
	//Send ack to the client
	send = "WR";
	write( sock, send, strlen( send ) );
	
	//Receive a send from client
	while( ( read_size = recv( sock, buffer, 512, 0 ) ) > 0 )
 		parse_input( buffer );
		
	if ( read_size == -1 ) {
		fprintf( stderr, "ERROR in server_receive: recv failed!\n" );
		send = "ER";
	} else {
		send = "OK";
	}
	write( sock, send, strlen( send ) );
	
	free( socket_desc );
	return 0;
}

// http://www.yolinux.com/TUTORIALS/LinuxTutorialPosixThreads.html
void *uart_receive( void *ptr ) {
	struct termios options;
	int   rcount;
	char  buffer[256];
	char *dev = configFile.serialDevice;

	tty = open( dev, O_RDONLY ); // O_RDWR | O_NOCTTY | O_NDELAY );
	if ( tty == -1 ) {
		perror( dev );
		exit(EXIT_FAILURE);
	}
	
	signal( SIGALRM, (void(*)(int)) signal_alarm );
	
	printf( "Opening %s\n", dev );
	
	if ( tcgetattr( tty, &options ) < 0 ) {
		perror( "ERROR: Getting configuration" );
		close( tty );
		exit(EXIT_FAILURE);
	}
	
	options.c_cflag = B115200 | CS8 | CREAD | CLOCAL; // 8 data bits, enable receiver, local line 
	options.c_cflag &= ~PARENB | ~CSTOPB; // No parity, one stop bit 
	options.c_iflag &= ~( IXON | IXOFF | IXANY ); // Disable software flow control
	options.c_oflag = 0;
	options.c_lflag = ICANON; // Read line
	options.c_cc[VTIME] = 0; // No timeout clock for read
	options.c_cc[VMIN] = 1; // Wait for one character before reading another
	tcflush( tty, TCIFLUSH ); 
	tcsetattr( tty, TCSANOW, &options ); // Set tty with new settings immediately
	
	while ( !terminate ) {
		rcount = read( tty, buffer, sizeof( buffer ) );
		if ( rcount < 0 ) {
			perror( "Read" );
		} else if ( rcount > 4 ) {
			buffer[rcount-1] = '\0';
			parse_input( buffer );
		}
		// http://stackoverflow.com/questions/12777254/time-delay-in-c-usleep
		usleep( 200 );
	}
	
	close( tty );
}

// http://stackoverflow.com/questions/26839558/hex-char-to-int-conversion
unsigned char hex2char( unsigned char c ) {
	if ( '0' <= c && c <= '9' )
		return c - '0';
	if ( 'A' <= c && c <= 'F' )
		return c - 'A' + 10;
	if ( 'a' <= c && c <= 'f' )
		return c - 'a' + 10;
	return -1;
}

// http://stackoverflow.com/questions/2602823/in-c-c-whats-the-simplest-way-to-reverse-the-order-of-bits-in-a-byte
unsigned char reverse_8bits( unsigned char n ) {
	return ( reverse_bits_lookup[n&0x0F] << 4 | reverse_bits_lookup[n>>4] );
}

void parse_input( char *s ) {
	if ( strncmp( s, "[", 1 ) == 0 );
	else if ( strncmp( s, "OSV2", 4 ) == 0 )
		osv2_parse( s + 5 );
	else if ( strncmp( s, "VENT", 4 ) == 0 )
		vent_parse( s + 5 );
	else if ( strncmp( s, "MAND", 4 ) == 0 )
		mandolyn_parse( s + 5 );
	else if ( strncmp( s, "FINE", 4 ) == 0 )
		fineoffset_parse( s + 5 );
	else
		printf( "Not recognised: " );
	
	printf( "%s\n", s );
}

/**************************************************************************************************
 * OREGON SCIENTIFIC VER 2.1                                       
 * http://connectingstuff.net/blog/decodage-des-protocoles-oregon-scientific-sur-arduino-2/
 * http://cpansearch.perl.org/src/BEANZ/Device-RFXCOM-1.110800/lib/Device/RFXCOM/Decoder/Oregon.pm
 * http://www.disk91.com/2013/technology/hardware/oregon-scientific-sensors-with-raspberry-pi/
 * http://www.mattlary.com/2012/06/23/weather-station-project/
 * THGN132N https://github.com/phardy/WeatherStation
 *************************************************************************************************/

void osv2_parse( char *s ) {
	unsigned int  id;
	unsigned char channel, batt, rolling, humidity;
	float         temperature, rain;
	SensorType    type;
	
	// ID is suggested as all nibbles straight [0][1][2][3] (0x1A2D)
	// or nibbles switched [0][3][2][4] (0x1D20), leaving nibble [1] as preamble
	id = hex2char( s[0] ) << 12 | hex2char( s[1] ) << 8 | hex2char( s[2] ) << 4 | hex2char( s[3] );
	channel = hex2char( s[4] );
	if ( id == 0x1A2D && channel == 4 )
		channel = 3;
	batt = hex2char( s[9] ) == 0;
	rolling = hex2char( s[6] ) << 4 | hex2char( s[7] );
	
	// Temperature, humidity and time sensor
	if ( (id&0xFFF) == 0xACC ){	// RTGR328N
		id          = 0xCACC;	// 0x9ACC 0xAACC 0xBACC 0xCACC 0xDACC 
		type        = TEMPERATURE | HUMIDITY;
		temperature = osv2_temperature( s );
		humidity    = osv2_humidity( s );
		
	// Temperature & Humidity sensors
	} else if( id == 0xFA28 	// THGR810	CRC		CRC8 code?
			|| id == 0x1A2D		// THGR228N, THGN122N, THGN123N, THGR122NX, THGR238, THGR268
			|| id == 0xFA28 	// THGR810
			|| id == 0x1A3D 	// THGR918, THGRN228NX, THGN500
			|| id == 0xCA2C ) {	// THGR328N
		type        = TEMPERATURE | HUMIDITY;
		temperature = osv2_temperature( s );
		humidity    = osv2_humidity( s );
	
	// Temperature sensors
	} else if ( id == 0x0A4D	// THR128
			|| id == 0xEA4C ) {	// THWR288A, THN132N
		type        = TEMPERATURE;
		temperature = osv2_temperature( s );
	
	} else {
		type = UNDEFINED;
		return;
	}
	
	sensor *sptr = sensorLookup( "OSV2", id, channel, rolling, type, batt );
	if ( sptr ) {
		if ( type & TEMPERATURE )
			sensorTemperature( sptr, temperature );
		if ( type & HUMIDITY)
			sensorHumidity( sptr, humidity );
		if ( type & RAIN)
			sensorRain( sptr, rain );
	}
}

float osv2_temperature( char *s ) {
	float temp =  hex2char( s[10] ) * 10 
				+ hex2char( s[11] ) 
				+ hex2char( s[8] ) / 10;
	if ( hex2char( s[13] ) & 0x8 )
		return -temp;
	return temp;
}

unsigned char osv2_humidity( char *s ) {
	return hex2char( s[15] ) * 10 + hex2char( s[12] );
}

/**************************************************************************************************
 * VENTUS/AURIOL WEATHER STATION
 * http://www.tfd.hu/tfdhu/files/wsprotocol/auriol_protocol_v20.pdf
 *************************************************************************************************/

void vent_parse( char *s ) {
	unsigned int  id;
	unsigned char crc, batt, btn, temp, tmp2, humidity;
	SensorType    type;
	float         temperature, rain, wind, gust;
	short         dir;
	
	id   = (int) hex2char( s[0] ) << 4 | hex2char( s[1] );
	tmp2 = hex2char( s[2] );
	batt = ( tmp2 & 0x8 ) == 0;
	btn  = ( tmp2 & 0x1 ) == 1;
	crc  = hex2char( s[8] );
	temp = hex2char( s[3] );
	
	// Temperature & Humidity
	if ( ( tmp2 & 0x6 ) != 0x6 ) {
		type = TEMPERATURE | HUMIDITY;
		temp = hex2char( s[5] );
		temperature = ( reverse_bits_lookup[hex2char( s[3] )] 
				| reverse_bits_lookup[hex2char( s[4] )] << 4 
				| reverse_bits_lookup[temp & 0xE] << 8 );
		temperature /= 10;
		if ( temp & 0x1 )
			temperature -= 204.8;
		humidity = reverse_bits_lookup[hex2char( s[6] )] 
				+ reverse_bits_lookup[hex2char( s[7] )] * 10;
	
	// Average Wind Speed
	} else if ( ( temp & 0xF ) == 0x8 ) {
		type = WINDSPEED;
		wind = ( reverse_bits_lookup[hex2char( s[7] ) << 4 ]
				| reverse_bits_lookup[hex2char( s[6] )] ) / 5;
	
	// Wind Gust & Bearing
	} else if ( ( temp & 0xE ) == 0xE ) {
		type = WINDDIR | WINDGUST;
		gust = ( reverse_bits_lookup[hex2char( s[7] ) << 4 ]
				| reverse_bits_lookup[hex2char( s[6] )] ) / 5;
		dir  = ( hex2char( s[3] ) & 0x1 )
				| reverse_bits_lookup[hex2char( s[4] )] << 1
				| reverse_bits_lookup[hex2char( s[5] )] << 5;
	
	// Rain guage
	} else if ( ( temp & 0xF ) == 0xC ) {
		type = RAIN;
		rain = ( reverse_8bits( hex2char( s[4] ) << 4 | hex2char( s[5] ) )
			| reverse_8bits( hex2char( s[6] ) << 4 | hex2char( s[7] ) ) << 8 ) * .25;
	
	} else {
		type = UNDEFINED;
		return;
	}
	
	sensor *sptr = sensorLookup( "VENT", id, 0, 0, type, batt );
	if ( sptr ) {
		if ( type & TEMPERATURE )
			sensorTemperature( sptr, temperature );
		if ( type & HUMIDITY)
			sensorHumidity( sptr, humidity );
		if ( type & RAIN)
			sensorRain( sptr, rain );
		if ( type & WINDSPEED)
			sensorWind( sptr, wind, -1.0, -1 );
		if ( type & WINDGUST)
			sensorWind( sptr, -1.0, gust, dir );
	}
}

/**************************************************************************************************
 * MANDOLYN
 * https://github.com/NetHome/Coders/blob/master/src/main/java/nu/nethome/coders/decoders/UPMDecoder.java
 * https://gitorious.org/sticktools/protocols/source/4698e465843a0eddc4c7029759f9c1dc79d4aab8:mandolyn.c
 **************************************************************************************************/

void mandolyn_parse( char *s ) {
	unsigned char id, channel, batt, sec;
	float         pri;
	SensorType    type;
	
	id      = hex2char( s[1] );
	channel = hex2char( s[2] ) >> 2;	
	batt    = ( ( hex2char( s[3] ) & 0x8 ) == 0x8 ? 0 : 1 );
	pri     = ( ( hex2char( s[5] ) << 8 ) | ( hex2char( s[6] ) << 4 ) | hex2char( s[7] ) ) / 16.0 - 50;
	sec     = ( ( hex2char( s[3] ) & 0x07 ) << 4 ) | hex2char( s[4] );
	
	// Rain and wind not tested
	if ( id == 10 ) {
		if ( channel == 2 ) {
			type = WINDSPEED | WINDGUST;
			sensor *sptr = sensorLookup( "MAND", id, channel, 0, type, batt );
			sensorWind( sptr, pri, 0.0, sec );
		} else if ( channel == 3 ) {
			type = RAIN;
			sensor *sptr = sensorLookup( "MAND", id, channel, 0, type, batt );
			sensorRain( sptr, pri );
		}
	} else if ( sec ) {
		type = TEMPERATURE | HUMIDITY;
		sensor *sptr = sensorLookup( "MAND", id, channel, 0, type, batt );
		sensorTemperature( sptr, pri );
		sensorHumidity( sptr, sec );
	} else {
		type = HUMIDITY;
		sensor *sptr = sensorLookup( "MAND", id, channel, 0, type, batt );
		sensorTemperature( sptr, pri );
	}
}

/**************************************************************************************************
 * FINE OFFSET
 * https://github.com/NetHome/Coders/blob/master/src/main/java/nu/nethome/coders/decoders/FineOffsetDecoder.java
 * https://gitorious.org/sticktools/protocols/source/4698e465843a0eddc4c7029759f9c1dc79d4aab8:mandolyn.c
 **************************************************************************************************/

void fineoffset_parse( char *s ) {
	unsigned char id, rolling, batt, humidity;
	float         temp, rain;
	SensorType    type;
	
	id       = hex2char( s[0] );
	rolling  = ( hex2char( s[1] ) << 4 ) | hex2char( s[2] );
	temp     = ( hex2char( s[3] ) & 0x7 ) * 10 + hex2char( s[4] ) + hex2char( s[5] ) / 10.0;
	if ( !hex2char( s[3] & 0x8 ) )
		temp = -temp;
	humidity = hex2char( s[6] ) * 10 + hex2char( s[7] );
	
	if ( id = 3 ) {
		type = RAIN;
		rain = (float) ( hex2char( s[6] ) * 100 + hex2char( s[7] ) * 10 + hex2char( s[9] ) ) * 0.03;
	} else {
		if ( humidity )
			type = TEMPERATURE | HUMIDITY;
		else
			type = TEMPERATURE;
	}
	
	// Temperature and humidity not tested!
	sensor *sptr = sensorLookup( "MANDO", id, 0, rolling, type, 1 );
	if ( sptr ) {
		if ( type & TEMPERATURE )
			sensorTemperature( sptr, temp );
		if ( type & HUMIDITY)
			sensorHumidity( sptr, humidity );
		if ( type & RAIN)
			sensorRain( sptr, rain );
	}
}

