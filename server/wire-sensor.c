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
#include "weather-reader.h"

extern ConfigSettings configFile;
extern int pipeServer[2];
extern int pipeParser[2];

void wire_main() {
	int  rcount, fd;
	double temperature, pressure;
	char str[80];

	if ( configFile.sensor_i2c == '\0' || ( fd = open( configFile.sensor_i2c,  O_RDWR ) ) < 0 ) {
		#if _DEBUG > 1
			fprintf( stderr, "%s: Failed to open the i2c bus %s,  error : %d\n", 
					__func__, configFile.sensor_i2c, errno );
		#endif
	
	// I2C bus active
	} else {
		if ( i2c_bmp085( str ) == 0 ) {
			#if _DEBUG > 1
				fprintf( stderr, "%s: i2c_bmp085 data (%d b): \"%s\"\n", __func__, strlen(str), str );
			#endif
			if ( configFile.is_client ) {
				if ( ( rcount = write( pipeServer[1], str, strlen(str)+1 ) ) < 1 )
					fprintf( stderr, "ERROR in %s: pipeServer error %d - %s\n", __func__, errno, str );
			} else {
				if ( ( rcount = write( pipeParser[1], str, strlen(str)+1 ) ) < 1 )
					fprintf( stderr, "ERROR in %s: pipeParser error %d - %s\n", __func__, errno, str );
			}
		}
	}
	close ( fd );
}

void wire_test() {
	wire_test_i2();
}

static int wire_test_i2() {
	int fd;
	configFile.sensor_i2c = "/dev/i2c-0";
	if ( ( fd = open( configFile.sensor_i2c,  O_RDWR ) ) > -1 ) {
		#if _DEBUG > 1
			fprintf( stderr, "%s: sensor_i2c device=\"%s\"\n", __func__, configFile.sensor_i2c );
		#endif
		close( fd );
		return 1;
	}
	fprintf( stderr, "%s: sensor_i2c device=\"%s (%d)\"\n", __func__, configFile.sensor_i2c, fd );
	
	configFile.sensor_i2c = "/dev/i2c-1";
	if ( ( fd = open( configFile.sensor_i2c,  O_RDWR ) ) > -1 ) {
		#if _DEBUG > 1
			fprintf( stderr, "%s: sensor_i2c device=\"%s\"\n", __func__, configFile.sensor_i2c );
		#endif
		close( fd );
		return 1;
	}
	fprintf( stderr, "%s: sensor_i2c device=\"%s (%d)\"\n", __func__, configFile.sensor_i2c, fd );
	
	configFile.sensor_i2c = "\0";
	#if _DEBUG > 1
		fprintf( stderr, "%s: No sensor_i2c device!\n", __func__ );
	#endif
	return -1;
}

