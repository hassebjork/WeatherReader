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

void *uart_receive( void *ptr ) {
	struct termios options;
	int   rcount;
	char  buffer[256], *dev = configFile.serialDevice;
	
	tty = open( dev, O_RDONLY ); // O_RDWR | O_NOCTTY | O_NDELAY );
	if ( tty == -1 ) {
		perror( dev );
		exit(EXIT_FAILURE);
	}
	
	printf( "Opening %s\n", dev );
	
	if ( tcgetattr( tty, &options ) < 0 ) {
		fprintf( stderr, "ERROR in uart_receive: Configuring device\n" );
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
	
	while ( 1 ) {
		rcount = read( tty, buffer, sizeof( buffer ) );
		if ( rcount < 0 ) {
			fprintf( stderr, "ERROR in uart_receive: Read %d bytes\n", rcount );
		} else if ( rcount > 4 ) {
			buffer[rcount-1] = '\0';
#if _DEBUG > 1
			printf( "uart_receive: %s\n", buffer );
#endif
			if ( configFile.is_client ) {
 				if ( write( pipeServer[1], &buffer, rcount ) < 1 )
 					fprintf( stderr, "ERROR in uart_receive: Pipe error\n" );
			} else
				parse_input( buffer );
		}
		// http://stackoverflow.com/questions/12777254/time-delay-in-c-usleep
		usleep( 200 );
	}
	
	close( tty );
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

