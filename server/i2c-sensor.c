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
		fprintf( stderr, "%s: I2C Read (ADDR 0x%0x REG 0x%0x) failed with error: %d\n", __func__, addr, reg, errno );
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
		fprintf( stderr, "%s: I2C Write (ADDR 0x%0x REG 0x%0x) failed with error: %d\n", __func__, addr, reg, errno );
		return -1;
	}
	return 0;
}

// BMP085 & BMP180 Specific code

int i2c_bmp085( int fd, char *str ) {
	double temperature, pressure;
	uint8_t rValue[21];
	if ( i2c_read( fd, BMP085_ADDR, BMP085_REG_EEPROM, 22, rValue ) < 0 ) {
		fprintf( stderr, "%s: Calibration failed\n", __func__ );
		return -1;
	}
	short int ac1 = ( ( rValue[0] <<8 ) | rValue[1]  );
	short int ac2 = ( ( rValue[2] <<8 ) | rValue[3]  );
	short int ac3 = ( ( rValue[4] <<8 ) | rValue[5]  );
	unsigned short int ac4 = ( ( rValue[6] <<8 ) | rValue[7]  );
	unsigned short int ac5 = ( ( rValue[8] <<8 ) | rValue[9]  );
	unsigned short int ac6 = ( ( rValue[10]<<8 ) | rValue[11] );
	short int b1 = ( ( rValue[12]<<8 ) | rValue[13] );
	short int b2 = ( ( rValue[14]<<8 ) | rValue[15] );
// 	short int mb = ( ( rValue[16]<<8 ) | rValue[17] );
	short int mc = ( ( rValue[18]<<8 ) | rValue[19] );
	short int md = ( ( rValue[20]<<8 ) | rValue[21] );
	
	// Temperature
	if ( i2c_write( fd, BMP085_ADDR, BMP085_REG_CTRL, 0x2E ) < 0 ) return -1;	// Temperature command 0x2E
	sleepms ( 4 );
	if ( i2c_bmp085_wait( fd ) < 0 ) return -1;
	if ( i2c_read( fd, BMP085_ADDR, 0xF6, 2, rValue ) < 0 ) return -1;	// Read 16 bit reg MSB 0xF6-0xF7 LSB
	
	unsigned int ut = ( ( rValue[0] << 8 ) | rValue[1] );
	int x1 = ( ( ( int )ut - ( int )ac6 ) * ( int )ac5 ) >> 15;
	int x2 = ( ( int )mc << 11 ) / ( x1 + md );
	int b5 = x1 + x2;
	temperature = (double)( ( b5 + 8 ) >> 4 ) / 10;
	
	int x3, b3, b6, p;
	unsigned int b4, b7;

	// Calculate calibrated pressure in hPa						// Pressure commands 16-19 bit reads reg MSB 0xF6-0xF8 LSB
	if ( i2c_write( fd, BMP085_ADDR, BMP085_REG_CTRL, 0x34 ) < 0 ) return -1;		// Command Time	  Samples Name
	sleepms ( 4 );																	// 0x34 	4.5ms	3	ultra low power
	if ( i2c_bmp085_wait( fd ) < 0 ) return -1;										// 0x74 	7.5 ms	5	standard
	if ( i2c_read( fd, BMP085_ADDR, 0xF6, 3, rValue ) < 0 ) return -1;				// 0xB4 	13.5 ms	7	high resolution
																					// 0xF4 	25.5 ms	12	ultra high res
	ut = ( ( ( unsigned int ) rValue[0] << 16 ) | ( ( unsigned int ) rValue[1] << 8 ) | ( unsigned int ) rValue[2] ) >> ( 8 - 3 );
	b6 = b5 - 4000;
	x1 = ( b2 * ( b6 * b6 ) >> 12 ) >> 11;
	x2 = ( ac2 * b6 ) >> 11;
	x3 = x1 + x2;
	b3 = ( ( ( ( ( int )ac1 )*4 + x3 ) << 3 ) + 2 ) >> 2;

	x1 = ( ac3 * b6 ) >> 13;
	x2 = ( b1 * ( ( b6 * b6 ) >> 12 ) ) >> 16;
	x3 = ( ( x1 + x2 ) + 2 ) >> 2;
	b4 = ( ac4 * ( unsigned int )( x3 + 32768 ) ) >> 15;

	b7 = ( ( unsigned int )( ut - b3 ) * ( 50000 >> 3 ) );
	if ( b7 < 0x80000000 )
		p = ( b7 << 1 ) / b4;
	else
		p = ( b7 / b4 ) << 1;

	x1 = ( p >> 8 ) * ( p >> 8 );
	x1 = ( x1 * 3038 ) >> 16;
	x2 = ( -7357 * p ) >> 16;
	p += ( x1 + x2 + 3791 ) >> 4;
	pressure = ( ( double )p / 100 );
// 	double altitude = 44330 * ( 1 - pow( pressure / p0 ), 1/5.255 );	// Altitude in meters, p0 ground level pressure hPa
	sprintf( str, "{\"type\":\"BMP085\",\"id\":%d,\"ch\":%d,\"T\":%.1f,\"P\":%.2f}\n", (ac1 & 0xFF), (ac2 & 0xFF), temperature, pressure );
	return 0;
}

int i2c_bmp085_wait( int fd ) {
	uint8_t rValues[3];
	int counter = 0;
	do {	//Delay can now be reduced by checking that bit 5 of BMP085_REG_CTRL( 0xF4 ) == 0
		sleepms( 2 );
		if ( i2c_read( fd, BMP085_ADDR, BMP085_REG_CTRL, 1, rValues ) < 0 ) return -1;
			counter++;
	} while ( ( ( rValues[0] & 0x20 ) != 0 ) && counter < 20 );
	return 0;
}
