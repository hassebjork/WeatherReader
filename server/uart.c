/*************************************************************************
   uart.c

   This module communicates with the Arduino over serial USB.
   
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

#include "uart.h"

extern ConfigSettings configFile;
extern int pipeServer[2];
extern int pipeParser[2];

void *uart_receive( void *ptr ) {
	SerialDevice *sDev = (SerialDevice *) ptr;
	
	struct termios options;
	int   rcount;
	char  buffer[256];
	
	sDev->tty = open( sDev->name, O_RDONLY ); // O_RDWR | O_NOCTTY | O_NDELAY );
	if ( sDev->tty == -1 ) {
		perror( sDev->name );
		return;
	}
	
	if ( tcgetattr( sDev->tty, &options ) < 0 ) {
		fprintf( stderr, "ERROR in uart_receive #%d: Configuring device\n",  sDev->no );
		close( sDev->tty );
		return;
	}
	
	options.c_cflag = B115200 | CS8 | CREAD | CLOCAL; // 8 data bits, enable receiver, local line 
	options.c_cflag &= ~PARENB | ~CSTOPB; // No parity, one stop bit 
	options.c_iflag &= ~( IXON | IXOFF | IXANY ); // Disable software flow control
	options.c_oflag = 0;
	options.c_lflag = ICANON;   // Read line
	options.c_cc[VTIME] = 0;    // 25s5 (max) timeout clock for read
	options.c_cc[VMIN]  = 1;    // Wait for one character before reading another
	tcflush( sDev->tty, TCIFLUSH ); 
	tcsetattr( sDev->tty, TCSANOW, &options ); // Set tty with new settings immediately
	
	// Timeout: http://stackoverflow.com/a/2918709/4405465
	// http://www.unixwiz.net/techtips/termios-vmin-vtime.html
	// http://stackoverflow.com/questions/6947413/how-to-open-read-and-write-from-serial-port-in-c
	// http://www.tldp.org/HOWTO/Serial-Programming-HOWTO/x115.html
	rcount = read( sDev->tty, buffer, sizeof( buffer ) );
	if ( rcount >= 0 ) {
		if ( strncmp( buffer, "[WR]", 4 ) == 0 ) {			// Weather-Reader
			sDev->active = 1;
			sDev->type   = WEATHER_READER;
		} else if ( strncmp( buffer, "[WS]", 4 ) == 0 ) {	// Wired-Sensor
			sDev->active = 1;
			sDev->type   = WIRE_SENSOR;
		} else {
			sDev->active = 0;
			sDev->type   = UNDEFINED;
		}
	}
	
	printf( "#%d. Opened %s as \"%s\"\n", sDev->no, sDev->name, SERIAL_TYPES[sDev->type] );
	
	while ( configFile.run && sDev->active ) {
		rcount = read( sDev->tty, buffer, sizeof( buffer ) );
		if ( rcount < 0 ) {
			fprintf( stderr, "ERROR in uart_receive #%d: Read %d bytes\n", sDev->no, rcount );
		} else if ( rcount == 0 ) {
			fprintf( stderr, "ERROR in uart_receive #%d: Timeout\n", sDev->no, rcount );
		} else if ( rcount > 4 ) {
			buffer[rcount-1] = '\0';
#if _DEBUG > 1
			printf( "uart_receive (#%d):   \"%s\"\n", sDev->no, buffer );
#endif
			if ( configFile.is_client ) {
 				if ( write( pipeServer[1], &buffer, rcount ) < 1 )
 					fprintf( stderr, "ERROR in uart_receive #%d: pipeServer error\n", sDev->no );
			} else {
 				if ( write( pipeParser[1], &buffer, rcount ) < 1 )
 					fprintf( stderr, "ERROR in uart_receive #%d: pipeParser error\n", sDev->no );
			}
		}
		// http://stackoverflow.com/questions/12777254/time-delay-in-c-usleep
		usleep( 200 );
	}
	
	printf( "#%d. Closing %s\n", sDev->no, sDev->name );
	close( sDev->tty );
}

/**
 * Function to reset an Arduino connected over serialUSB. This is 
 * done by dropping the DTR signal.
 */
void reset_arduino( SerialDevice *sDev ) {
	// http://www.linuxtv.org/mailinglists/vdr/2003/02-2003/msg00543.html
	int flag = TIOCM_DTR;
	ioctl( sDev->tty, TIOCMBIC, &flag );
	sleep( 1 );
	ioctl( sDev->tty, TIOCMBIS, &flag );
#if _DEBUG > 1
	char s[20];
	printTime( s );
	fprintf( stderr, "%s Resetting Arduino\n", s );
#endif
}

/**
 * Scan /dev for a serialUSB devices (ttyUSBx)
 */
char ** uart_get_serial( void ) {
	DIR   *d;
	struct dirent *dir;
	int    elem  = 0;
	char **temp, **array;
	char   str[25], *dir_dev = "/dev/";
	
	array    = malloc( sizeof( char *) );
	if ( array ) {
		array[0] = NULL;
	} else  {
		fprintf( stderr, "ERROR in get_serial: Could not malloc\n" );
		return NULL;
	}
	
	d = opendir( dir_dev );
	if ( d && array ) {
		while ( ( dir = readdir(d) ) != NULL ) {
			if ( dir->d_type == DT_CHR && strncmp( dir->d_name, "ttyUSB", 6 ) == 0 ) {
				temp = realloc( array, sizeof( *array ) * ( ++elem ) );
				if ( temp ) {
					array = temp;
				} else {
					fprintf( stderr, "ERROR in get_serial: Could not allocate array\n" );
					elem--;
				}
				strcpy( str, dir_dev );
				strcat( str, dir->d_name );
				array[elem-1] = strdup( str );
				array[elem]   = NULL;
			}
		}
		closedir(d);
	} else {
		fprintf( stderr, "ERROR in get_serial: Could not open directory /dev\n" );
	}
	return array;
}