#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <signal.h>
#include "weatherreader.h"

#define DEV_UNDEFINED 0
#define DEV_TEMP      1
#define DEV_TEMPHUMID 2
#define DEV_WINDSPEED 3
#define DEV_WINDGUST  4
#define DEV_WINDDIR   5
#define DEV_RAIN      6

int terminate = 0;
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

// http://connectingstuff.net/blog/decodage-des-protocoles-oregon-scientific-sur-arduino-2/
void osv2_parse( char *s ) {
	unsigned int id;
	unsigned char channel, batt, rolling;
	
	id = hex2char( s[0] ) << 12 | hex2char( s[1] ) << 8 | hex2char( s[2] ) << 4 | hex2char( s[3] );
	channel = hex2char( s[4] );
	if ( id == 0x1A2D && channel == 4 )
		channel = 3;
	batt = hex2char( s[5] );
	rolling = hex2char( s[6] ) << 4 | hex2char( s[7] );

	printf( "Id: %X Ch: %d:%X Batt: %d ", id, channel, rolling, batt );

	if ( id == 0x1A2D || ( id&0xFFF ) == 0xACC ) {
		float temperature = hex2char( s[10] ) * 10 + hex2char( s[11] ) + hex2char( s[8] ) / 10;
		if ( hex2char( s[13] ) & 0x8 )
			temperature = -temperature;
		unsigned char humidity = hex2char( s[15] ) * 10 + hex2char( s[12] );
		printf( "Temp: %.1f Humid: %d\n", temperature, humidity );
	} else {
		printf( "\n" );
	}
}
// ID id CB RR t9 Tt h±  H CS CRC
// 1A 2D 10 D6 22 05 68 C8 4F A5
// 01 23 45 67 89 01 23 45 67 89
// float get_os_temperature(const unsigned char *message) {
// 	float temp_c = (message[5]>>4)*10+(message[5]&0x0f) + (message[4]>>4)/10.0F;
// 	if (message[6] & 0x08)
// 		return -temp_c;
// 	return temp_c;
// }
// unsigned int get_os_humidity(const unsigned char *message) {
// 	int humidity = ((message[7]&0x0f)*10)+(message[6]>>4);
// 	return humidity;
// }

void vent_parse( char *s ) {
	unsigned char id, crc, batt, btn, temp, type;
	
	id   = hex2char( s[0] ) << 4 | hex2char( s[1] );
	type = hex2char( s[2] );
	batt = ( type & 0x8 == 0x8 );
	btn  = ( type & 0x1 == 1 );
	crc  = hex2char( s[8] );
	temp = hex2char( s[3] );
	
	printf( "Id: %X Batt: %s CheckSum: %X ", id, ( batt ? "OK" : "Low" ), crc );
	
	if ( ( type & 0x6 ) != 0x6 ) {
		type = DEV_TEMPHUMID;
		temp = hex2char( s[5] );
		float temperature = ( reverse_bits_lookup[hex2char( s[3] )] 
				| reverse_bits_lookup[hex2char( s[4] )] << 4 
				| reverse_bits_lookup[temp & 0xE] << 8 );
		temperature /= 10;
		if ( temp & 0x1 )
			temperature -= 204.8;
		unsigned char humidity = reverse_bits_lookup[hex2char( s[6] )] 
				+ reverse_bits_lookup[hex2char( s[7] )] * 10;
		printf( "Temp: %.1f Humid: %d\n", temperature, humidity );
	} else if ( ( temp & 0xF ) == 0x8 ) {
		type = DEV_WINDSPEED;
		float wind = ( reverse_bits_lookup[hex2char( s[7] ) << 4 ]
				| reverse_bits_lookup[hex2char( s[6] )] ) / 5;
		printf( "Wind Speed: %.1f\n", wind );
	} else if ( ( temp & 0xF ) == 0xC ) {
		type = DEV_RAIN;
		float rain = ( reverse_8bits( hex2char( s[4] ) << 4 | hex2char( s[5] ) )
			| reverse_8bits( hex2char( s[6] ) << 4 | hex2char( s[7] ) ) << 8 ) * .25;
		printf( "Rain: %.2f\n", rain );
	} else if ( ( temp & 0xE ) == 0xE ) {
		type = DEV_WINDGUST;
		float gust = ( reverse_bits_lookup[hex2char( s[7] ) << 4 ]
				| reverse_bits_lookup[hex2char( s[6] )] ) / 5;
		short dir = ( hex2char( s[3] ) & 0x1 )
				| reverse_bits_lookup[hex2char( s[4] )] << 1
				| reverse_bits_lookup[hex2char( s[5] )] << 5;
		printf( "Wind Gust: %.1f Dir: %d\n", gust, dir );
	}
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
