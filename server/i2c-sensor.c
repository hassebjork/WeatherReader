/*************************************************************************
   i2c-sensor.c

   This module decodes various sensors connected to the I2C bus.
   Supported sensors: BMP085, BMP180
   
   This module requires the kernel module i2c-dev to be loaded
   
   Configuration is done in the file weather-reader.conf
   
   This file is part of the Weather Station reader program WeatherReader.
   This module has been modified from these sources:
      https://github.com/maasdoel/bmp180
      http://bunniestudios.com/blog/images/infocast_i2c.c
   
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
#include "i2c-sensor.h"


static int i2c_read(int file, uint8_t addr, uint8_t reg, uint8_t bytes, uint8_t *val ) {
	struct i2c_rdwr_ioctl_data packets;
	struct i2c_msg messages[2];
	
	/*
		* In order to read a register, we first do a "dummy write" by writing
		* 0 bytes to the register we want to read from.  This is similar to
		* the packet in i2c_write, except it's 1 byte rather than 2.
		*/
	messages[0].addr  = addr;
	messages[0].flags = 0;
	messages[0].len   = 1;
	messages[0].buf   = &reg;

	/* The data will get returned in this structure */
	messages[1].addr  = addr;
	messages[1].flags = I2C_M_RD/* | I2C_M_NOSTART*/;
	messages[1].len   = bytes;
	messages[1].buf   = val;

	/* Send the request to the kernel and get the result back */
	packets.msgs      = messages;
	packets.nmsgs     = 2;
	if( ioctl( file, I2C_RDWR, &packets ) < 0 ) {
		fprintf( stderr, "I2C Read (ADDR 0x%0x REG 0x%0x) failed with error: %d\n", addr, reg, errno );
		return -1;
	}
	return 0;
}

static int i2c_write(int file, uint8_t addr, uint8_t reg, uint8_t value ) {
	uint8_t outbuf[2];
	struct i2c_rdwr_ioctl_data packets;
	struct i2c_msg messages[1];

	messages[0].addr  = addr;
	messages[0].flags = 0;
	messages[0].len   = sizeof(outbuf);
	messages[0].buf   = outbuf;
	outbuf[0] = reg;
	outbuf[1] = value;

	/* Transfer the i2c packets to the kernel and verify it worked */
	packets.msgs  = messages;
	packets.nmsgs = 1;
	if( ioctl( file, I2C_RDWR, &packets ) < 0 ) {
		fprintf( stderr, "I2C Write (ADDR 0x%0x REG 0x%0x) failed with error: %d\n", addr, reg, errno );
		return -1;
	}
	return 0;
}

// BMP085 & BMP180 Specific code

int i2c_bmp085( int fd, double *temperature, double *pressure ) {
	uint8_t rValue[21];
	if ( i2c_read( fd, 0x77, 0xAA, 22, rValue ) < 0 ) {
		fprintf( stderr, "Calibration failed\n" );
		return -1;
	}
	short int bmp_ac1 = ( ( rValue[0] <<8 ) | rValue[1]  );
	short int bmp_ac2 = ( ( rValue[2] <<8 ) | rValue[3]  );
	short int bmp_ac3 = ( ( rValue[4] <<8 ) | rValue[5]  );
	unsigned short int bmp_ac4 = ( ( rValue[6] <<8 ) | rValue[7]  );
	unsigned short int bmp_ac5 = ( ( rValue[8] <<8 ) | rValue[9]  );
	unsigned short int bmp_ac6 = ( ( rValue[10]<<8 ) | rValue[11] );
	short int bmp_b1 = ( ( rValue[12]<<8 ) | rValue[13] );
	short int bmp_b2 = ( ( rValue[14]<<8 ) | rValue[15] );
// 	short int bmp_mb = ( ( rValue[16]<<8 ) | rValue[17] );
	short int bmp_mc = ( ( rValue[18]<<8 ) | rValue[19] );
	short int bmp_md = ( ( rValue[20]<<8 ) | rValue[21] );
	
	// Temperature
	if ( i2c_write( fd, 0x77, 0xF4, 0x2E ) < 0 ) return -1;
	sleepms ( 4 );
	if ( i2c_bmp085_wait( fd ) < 0 ) return -1;
	if ( i2c_read( fd, 0x77, 0xF6, 2, rValue ) < 0 ) return -1;
	
	unsigned int ut = ( ( rValue[0] << 8 ) | rValue[1] );
	int x1 = ( ( ( int )ut - ( int )bmp_ac6 ) * ( int )bmp_ac5 ) >> 15;
	int x2 = ( ( int )bmp_mc << 11 ) / ( x1 + bmp_md );
	int bmp_b5 = x1 + x2;
	*temperature = (double)( ( bmp_b5 + 8 ) >> 4 ) / 10;
	
	int x3, b3, b6, p;
	unsigned int b4, b7;

	// Calculate calibrated pressure in hPa
	if ( i2c_write( fd, 0x77, 0xF4, ( 0x34 + ( 3 << 6 ) ) ) < 0 ) return -1;
	sleepms ( 4 );
	if ( i2c_bmp085_wait( fd ) < 0 ) return -1;
	if ( i2c_read( fd, 0x77, 0xF6, 3, rValue ) < 0 ) return -1;
	
	ut = ( ( ( unsigned int ) rValue[0] << 16 ) | ( ( unsigned int ) rValue[1] << 8 ) | ( unsigned int ) rValue[2] ) >> ( 8 - 3 );
	b6 = bmp_b5 - 4000;
	x1 = ( bmp_b2 * ( b6 * b6 ) >> 12 ) >> 11;
	x2 = ( bmp_ac2 * b6 ) >> 11;
	x3 = x1 + x2;
	b3 = ( ( ( ( ( int )bmp_ac1 )*4 + x3 ) << 3 ) + 2 ) >> 2;

	x1 = ( bmp_ac3 * b6 ) >> 13;
	x2 = ( bmp_b1 * ( ( b6 * b6 ) >> 12 ) ) >> 16;
	x3 = ( ( x1 + x2 ) + 2 ) >> 2;
	b4 = ( bmp_ac4 * ( unsigned int )( x3 + 32768 ) ) >> 15;

	b7 = ( ( unsigned int )( ut - b3 ) * ( 50000 >> 3 ) );
	if ( b7 < 0x80000000 )
		p = ( b7 << 1 ) / b4;
	else
		p = ( b7 / b4 ) << 1;

	x1 = ( p >> 8 ) * ( p >> 8 );
	x1 = ( x1 * 3038 ) >> 16;
	x2 = ( -7357 * p ) >> 16;
	p += ( x1 + x2 + 3791 ) >> 4;
	*pressure = ( ( double )p / 100 );
	
	return 0;
}

int i2c_bmp085_wait( int fd ) {
	uint8_t rValues[3];
	int counter=0;
	//Delay can now be reduced by checking that bit 5 of Ctrl_Meas( 0xF4 ) == 0
	do {
		sleepms( 2 );
		if ( i2c_read( fd, 0x77, 0xF4, 1, rValues ) < 0 ) return -1;
			counter++;
	} while ( ( ( rValues[0] & 0x20 ) != 0 ) && counter < 20 );
	return 0;
}

// double bmp_altitude( double p ) {
// 	return 145437.86 * ( 1 - pow( ( p / 1013.25 ), 0.190294496 ) ); //return feet
// }
// 
// double bmp_qnh( double p, double StationAlt ) {
// 	return p / pow( ( 1 - ( StationAlt / 145437.86 ) ), 5.255 ); //return hPa based on feet
// }
// 
// double ppl_DensityAlt( double PAlt, double Temp ) {
// 	double ISA = 15 - ( 1.98 * ( PAlt / 1000 ) );
// 	return PAlt + ( 120 * ( Temp - ISA ) ); //So, So density altitude
// }

// int main( int argc,  char **argv ) {
// 	if ( ( fd = open( I2CBus,  O_RDWR ) ) < 0 ) {
// 		fprintf( stderr, "%s: Failed to open the i2c bus %s,  error : %d\n", __func__, I2CBus, errno );
// 		exit( 1 );      //Use this line if the function must terminate on failure
// 	}
// 	
// 	if ( i2c_bmp085( fd, &temperature, &pressure ) < 0 )
// 		fprintf( stderr, "Combined reading failed\n" );
// 	printf ( "Temperature: %.1f C\n", temperature );
// 	printf ( "Pressure: %.2f hPa\n", pressure );
// 	close ( fd );
// 
// 	return 0;
//  }