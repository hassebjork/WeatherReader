/*************************************************************************
   wire-sensor.c

   This module fetches data from sensors wired to the Raspberry Pi.
   
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

#include "wire-sensor.h"

extern ConfigSettings configFile;
extern int pipeServer[2];

void *wire_thread() {
	char buffer[254], *s;
	int  rcount, fd;
	double temperature, pressure;

#if _DEBUG > 0
	fprintf( stderr, "Wire thread:%16sRunning\n", "" );
#endif
	
	if ( ( fd = open( I2CBus,  O_RDWR ) ) < 0 ) {
		fprintf( stderr, "%s: Failed to open the i2c bus %s,  error : %d\n", __func__, I2CBus, errno );
		exit( 1 );      //Use this line if the function must terminate on failure
	}
	
	if ( i2c_bmp085( fd, &temperature, &pressure ) < 0 )
		fprintf( stderr, "Combined reading failed\n" );
	
	printf ( "Temperature: %.1f C\n", temperature );
	printf ( "Pressure: %.2f hPa\n", pressure );
	close ( fd );
	
// 	if ( ( rcount = write( pipeServer[1], s, rcount ) ) < 1 )
// 		fprintf( stderr, "ERROR in wire_thread: Sending data.\n" );
}

void wire_main() {
	int  rcount, fd;
	double temperature, pressure;

	if ( ( fd = open( I2CBus,  O_RDWR ) ) < 0 ) {
		fprintf( stderr, "%s: Failed to open the i2c bus %s,  error : %d\n", __func__, I2CBus, errno );
	} else {
		if ( i2c_bmp085( fd, &temperature, &pressure ) < 0 )
			fprintf( stderr, "Combined reading failed\n" );
		printf ( "Temperature: %.1f C\n", temperature );
		printf ( "Pressure: %.2f hPa\n", pressure );
	}
	close ( fd );
	
// 	if ( ( rcount = write( pipeServer[1], s, rcount ) ) < 1 )
// 		fprintf( stderr, "ERROR in wire_thread: Sending data.\n" );
}

