#include "weatherreader.h"

int terminate = 0;
extern ConfigSettings configFile;

static unsigned char reverse_bits_lookup[16] = {
	0x0, 0x8, 0x4, 0xC, 0x2, 0xA, 0x6, 0xE,
	0x1, 0x9, 0x5, 0xD, 0x3, 0xB, 0x7, 0xF
};

void signal_callback_handler( int signum ) {
	printf( "\nCaught signal %d\nExiting!\n", signum );
	terminate = 1;
	exit( EXIT_SUCCESS );
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

// http://connectingstuff.net/blog/decodage-des-protocoles-oregon-scientific-sur-arduino-2/
// http://cpansearch.perl.org/src/BEANZ/Device-RFXCOM-1.110800/lib/Device/RFXCOM/Decoder/Oregon.pm
void osv2_parse( char *s ) {
	unsigned int  id;
	unsigned char channel, batt, rolling;
	SensorType    type;
	
	id = hex2char( s[0] ) << 12 | hex2char( s[1] ) << 8 | hex2char( s[2] ) << 4 | hex2char( s[3] );
	channel = hex2char( s[4] );
	if ( id == 0x1A2D && channel == 4 )
		channel = 3;
	batt = hex2char( s[9] ) == 0;
	rolling = hex2char( s[6] ) << 4 | hex2char( s[7] );

#if _DEBUG > 2
	printf( "Id: %X Ch: %d:%X ", id, channel, rolling );
#endif
/*
	http://www.disk91.com/2013/technology/hardware/oregon-scientific-sensors-with-raspberry-pi/
	http://www.mattlary.com/2012/06/23/weather-station-project/
	
	THGN132N https://github.com/phardy/WeatherStation
	
	Temperature & Humidity sensors		IDid	Product identifier
		ID id Ci RR vB Tt h+ ?H CS CRC		C		Channel (some sensors 4=>3)
		1A 2D 10 D6 22 05 68 C8 4F A5		i		ID LSB? then 2nd char (A) is preample
		1A 2D 20 72 40 18 80 83 3B 98		RR		Rolling code
		1A 2D 40 2D 10 07 40 06 35 E9		B		Battery 0=OK
		CA CC 53 7D 70 19 50 83 61 1C		+Ttv	Temperature BCD +Tt.v (0x0=pos 0x8=neg)
											Hh		Humidity BCD Hh		*/
	if ( (id&0xFFF) == 0xACC ){	// RTGR328N	CS		CheckSun
		id   = 0xCACC;	// 0x9ACC 0xAACC 0xBACC 0xCACC 0xDACC 
		type = TEMPERATURE | HUMIDITY;
		float      temperature = osv2_temperature( s );
		unsigned char humidity = osv2_humidity( s );
#if _DEBUG > 2
		printf( "Temp: %.1f Humid: %d Batt: %d \n", temperature, humidity, batt );
#endif
		
	// Temperature & Humidity sensors
	} else if( id == 0xFA28 	// THGR810	CRC		CRC8 code?
			|| id == 0x1A2D		// THGR228N, THGN122N, THGN123N, THGR122NX, THGR238, THGR268
			|| id == 0xFA28 	// THGR810
			|| id == 0x1A3D 	// THGR918, THGRN228NX, THGN500
			|| id == 0xCA2C ) {	// THGR328N
		type = TEMPERATURE | HUMIDITY;
		float      temperature = osv2_temperature( s );
		unsigned char humidity = osv2_humidity( s );
#if _DEBUG > 2
		printf( "Temp: %.1f Humid: %d Batt: %d \n", temperature, humidity, batt );
#endif
	
	// Temperature sensors
	} else if ( id == 0x0A4D	// THR128
			|| id == 0xEA4C ) {	// THWR288A, THN132N
		type = TEMPERATURE;
		float temperature = osv2_temperature( s );
#if _DEBUG > 2
		printf( "Temp: %.1f Batt: %d \n", temperature, batt );
#endif
	
	} else {
		type = UNDEFINED;
#if _DEBUG > 2
		printf( "Not identified!\n" );
#endif
		return;
	}
	
	sensor *sptr = sensorLookup( "OSV2", id, channel, rolling, type, batt );
}

void vent_parse( char *s ) {
	unsigned int  id;
	unsigned char crc, batt, btn, temp, tmp2;
	SensorType    type;
	
	id   = (int) hex2char( s[0] ) << 4 | hex2char( s[1] );
	tmp2 = hex2char( s[2] );
	batt = ( tmp2 & 0x8 ) == 0;
	btn  = ( tmp2 & 0x1 ) == 1;
	crc  = hex2char( s[8] );
	temp = hex2char( s[3] );
	
#if _DEBUG > 2
	printf( "Id: %X Batt: %X CheckSum: %X ", id, batt, crc );
#endif
	
	// Temperature & Humidity
	if ( ( tmp2 & 0x6 ) != 0x6 ) {
		type = TEMPERATURE | HUMIDITY;
		temp = hex2char( s[5] );
		float temperature = ( reverse_bits_lookup[hex2char( s[3] )] 
				| reverse_bits_lookup[hex2char( s[4] )] << 4 
				| reverse_bits_lookup[temp & 0xE] << 8 );
		temperature /= 10;
		if ( temp & 0x1 )
			temperature -= 204.8;
		unsigned char humidity = reverse_bits_lookup[hex2char( s[6] )] 
				+ reverse_bits_lookup[hex2char( s[7] )] * 10;
#if _DEBUG > 2
		printf( "Temp: %.1f Humid: %d\n", temperature, humidity );
#endif
	
	// Average Wind Speed
	} else if ( ( temp & 0xF ) == 0x8 ) {
		type = WINDSPEED;
		float wind = ( reverse_bits_lookup[hex2char( s[7] ) << 4 ]
				| reverse_bits_lookup[hex2char( s[6] )] ) / 5;
#if _DEBUG > 2
		printf( "Wind Speed: %.1f\n", wind );
#endif
	
	// Wind Gust & Bearing
	} else if ( ( temp & 0xE ) == 0xE ) {
		type = WINDDIR | WINDGUST;
		float gust = ( reverse_bits_lookup[hex2char( s[7] ) << 4 ]
				| reverse_bits_lookup[hex2char( s[6] )] ) / 5;
		short dir = ( hex2char( s[3] ) & 0x1 )
				| reverse_bits_lookup[hex2char( s[4] )] << 1
				| reverse_bits_lookup[hex2char( s[5] )] << 5;
#if _DEBUG > 2
		printf( "Wind Gust: %.1f Dir: %d\n", gust, dir );
#endif
	
	// Rain guage
	} else if ( ( temp & 0xF ) == 0xC ) {
		type = RAIN;
		float rain = ( reverse_8bits( hex2char( s[4] ) << 4 | hex2char( s[5] ) )
			| reverse_8bits( hex2char( s[6] ) << 4 | hex2char( s[7] ) ) << 8 ) * .25;
#if _DEBUG > 2
		printf( "Rain: %.2f\n", rain );
#endif
	
	} else {
		type = UNDEFINED;
#if _DEBUG > 2
		printf( "Not identified!\n" );
#endif
		return;
	}
	
	sensor *sptr = sensorLookup( "VENT", id, 0, 0, type, batt );
}

void parse_input( char *s ) {
	char *str = malloc( sizeof(char) * ( strlen( s ) + 1 ) );
	if ( str == NULL )
		perror( "Malloc: Out of memory!" );
	strcpy( str, s );
	
	if ( strncmp( str, "OSV2", 4 ) == 0 )
		osv2_parse( str + 5 );
	else if ( strncmp( str, "VENT", 4 ) == 0 )
		vent_parse( str + 5 );
	
	printf( "%s\n", str );
	free( str );
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
			parse_input( buffer );
		}
		// http://stackoverflow.com/questions/12777254/time-delay-in-c-usleep
		usleep( 200 );
	}
	
	close( fd );
}

int main( int argc, char *argv[]) {
	pthread_t thread1;
	int iret1;
	
	confReadFile( CONFIG_FILE_NAME, &configFile );
	sensorInit();
	
	/* Create independent threads each of which will execute function */
	iret1 = pthread_create( &thread1, NULL, uart_receive, (void*) configFile.serialDevice );
	if ( iret1 ) {
		fprintf( stderr, "ERROR in main: pthread_create() return code: %d\n", iret1 );
		exit(EXIT_FAILURE);
	}

	signal( SIGINT, signal_callback_handler );

	/* Wait till threads are complete before main continues.  */
	pthread_join( thread1, NULL);

	exit(EXIT_SUCCESS);
}
