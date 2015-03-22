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

/**
 * UART-thread for receiving data from arduino connected by USB-serial cable
 */
void *uart_receive( void *ptr ) {
	SerialDevice *sDev = (SerialDevice *) ptr;
	
	struct termios options;
	int   rcount;
	char  buffer[256];
	
#if _DEBUG > 1
	fprintf( stderr, "Uart thread #%d %s:%*sEnabled\n", sDev->no, sDev->name, ( 14 - strlen( sDev->name ) ), "" );
#endif
	
	// Open device
	sDev->tty = open( sDev->name, O_RDONLY ); // O_RDWR | O_NOCTTY | O_NDELAY );
	if ( sDev->tty == -1 ) {
		perror( sDev->name );
		return;
	}
	
	// Get communication flags
	if ( tcgetattr( sDev->tty, &options ) < 0 ) {
		fprintf( stderr, "ERROR in uart_receive #%d: Configuring device\n",  sDev->no );
		close( sDev->tty );
		return;
	}
	
	// Set communication flags
	options.c_cflag = B115200 | CS8 | CREAD | CLOCAL; // 8 data bits, enable receiver, local line 
	options.c_cflag &= ~PARENB | ~CSTOPB; // No parity, one stop bit 
	options.c_iflag &= ~( IXON | IXOFF | IXANY ); // Disable software flow control
	options.c_oflag = 0;
	options.c_lflag = ICANON;   // Read line
	options.c_cc[VTIME] = 0;    // 25s5 (max) timeout clock for read
	options.c_cc[VMIN]  = 1;    // Wait for one character before reading another
	tcflush( sDev->tty, TCIFLUSH ); 
	tcsetattr( sDev->tty, TCSANOW, &options ); // Set tty with new settings immediately
	
	// Fetch id string from arduino. Reset and retry on fail.
	uart_init( sDev );
	if ( sDev->type == UNDEFINED ) {
		reset_arduino( sDev );
		uart_init( sDev );
	}
	
	printf( "Opened #%d %s%*s\"%s\" found\n", sDev->no, sDev->name, ( 20 - strlen( sDev->name ) ), "", SERIAL_TYPES[sDev->type] );
	
	// Read serial port loop
	while ( configFile.run && sDev->active ) {
		rcount = read( sDev->tty, buffer, sizeof( buffer ) );
		if ( rcount < 0 ) {
			fprintf( stderr, "ERROR in uart_receive #%d: Read %d bytes\n", sDev->no, rcount );
		} else if ( rcount == 0 ) {
			fprintf( stderr, "ERROR in uart_receive #%d: Timeout\n", sDev->no, rcount );
		} else if ( rcount > 4 ) {
			buffer[rcount-1] = '\0';
			
			// Send data through client thread to remote server
			if ( configFile.is_client ) {
				if ( write( pipeServer[1], &buffer, rcount ) < 1 )
					fprintf( stderr, "ERROR in uart_receive #%d: pipeServer error\n", sDev->no );
#if _DEBUG > 1
				fprintf( stderr, "uart_receive #%d:%*s\"%s\" sent %d bytes to parser\n", sDev->no, 14, "", buffer, rcount );
#endif
			
			// Send data to parser thread and database
			} else {
				if ( write( pipeParser[1], &buffer, rcount ) < 1 )
					fprintf( stderr, "ERROR in uart_receive #%d: pipeParser error\n", sDev->no );
#if _DEBUG > 1
				fprintf( stderr, "uart_receive #%d:%*s\"%s\" sent %d bytes to client\n", sDev->no, 4, "", buffer, rcount );
#endif
			}
		}
	}
	
	close( sDev->tty );
	free( sDev );
#if _DEBUG > 1
	fprintf( stderr, "Uart thread #%d %s:%*sClosing\n", sDev->no, sDev->name, 14 - strlen( sDev->name ), "" );
#endif
}

/**
 * Read first identifier string from arduino and select type
 */
void uart_init( SerialDevice *sDev ) {
	sleep( 1 );			// Wait for arduino to boot
	char  buffer[256];
	int   rcount = read( sDev->tty, buffer, sizeof( buffer ) );
	if ( rcount >= 0 ) {
		if ( strncmp( buffer, "[WR]", 4 ) == 0 ) {			// Weather-Reader
			sDev->active = 1;
			sDev->type   = WEATHER_READER;
		} else if ( strncmp( buffer, "[WS]", 4 ) == 0 ) {	// Wired-Sensor
			sDev->active = 1;
			sDev->type   = WIRE_SENSOR;
		} else {											// Undefined - quit thread
			sDev->active = 0;
			sDev->type   = UNDEFINED;
		}
	}
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
	fprintf( stderr, "reset_arduino #%d %s%*sResetting Arduino\n", sDev->no, sDev->name, 13 - strlen( sDev->name ), "" );
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
	
	// Allocate memory for string array
	array    = malloc( sizeof( char *) );
	if ( array ) {
		array[0] = NULL;
	} else  {
		fprintf( stderr, "ERROR in get_serial: Could not malloc\n" );
		return NULL;
	}
	
	// Scan directory /dev
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