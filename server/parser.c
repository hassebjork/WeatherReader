/*************************************************************************
   parser.c

   This module decodes the radio codes from the Arduino.
   
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

#include "parser.h"

extern ConfigSettings configFile;
extern int pipeParser[2];

static unsigned char reverse_bits_lookup[16] = {
	0x0, 0x8, 0x4, 0xC, 0x2, 0xA, 0x6, 0xE,
	0x1, 0x9, 0x5, 0xD, 0x3, 0xB, 0x7, 0xF
};

void *parse_thread() {
	char buffer[254], *s;
	int  rcount;

#if _DEBUG > 0
	fprintf( stderr, "Parser thread:%16sRunning\n", "" );
#endif
	sensorInit();
	
	while ( ( rcount = read( pipeParser[0], &buffer, 254 ) ) > 0 && configFile.run ) {
		buffer[rcount-1] = '\0';
		// Split multiple inputs
		s = buffer;
		while( s < buffer + rcount ) {
			parse_input( s );
			s = strchr( s, 0x0 ) + 1;
		}
	}
	
	if ( rcount == 0 )
		fprintf( stderr, "ERROR in parse_thread: Pipe closed\n" );
	else
		fprintf( stderr, "ERROR in parse_thread: Pipe error %d\n", rcount );
	exit(EXIT_FAILURE);
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
#if _DEBUG > 1
	fprintf( stderr, "parse_input:%8s\"%s\" scan %d bytes\n", "", s, strlen( s ) );
#endif
	if ( strncmp( s, "[", 1 ) == 0 )
		return;
	else if ( strncmp( s, "OSV2", 4 ) == 0 )
		osv2_parse( s + 5 );
	else if ( strncmp( s, "VENT", 4 ) == 0 )
		vent_parse( s + 5 );
	else if ( strncmp( s, "MAND", 4 ) == 0 )
		mandolyn_parse( s + 5 );
	else if ( strncmp( s, "FINE", 4 ) == 0 )
		fineoffset_parse( s + 5 );
	else if ( strncmp( s, "WIRE", 4 ) == 0 )
		wired_parse( s + 5 );
#if _DEBUG > 2
	else
		fprintf( stderr, "Not recognised: %s", s );
#endif
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
	
	sensor *sptr = sensorListLookup( "OSV2", id, channel, rolling, type, batt );
	if ( sptr ) {
		if ( type & TEMPERATURE )
			sensorTemperature( sptr, temperature );
		if ( type & HUMIDITY)
			sensorHumidity( sptr, humidity );
		if ( type & RAINTOTAL)
			sensorRain( sptr, rain );
	}
}

float osv2_temperature( char *s ) {
	float temp =  hex2char( s[10] ) * 10 
				+ hex2char( s[11] ) 
				+ hex2char( s[8] ) / 10.0;
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
		type = RAINTOTAL;
		rain = ( reverse_8bits( hex2char( s[4] ) << 4 | hex2char( s[5] ) )
			| reverse_8bits( hex2char( s[6] ) << 4 | hex2char( s[7] ) ) << 8 ) * .25;
	
	} else {
		type = UNDEFINED;
		return;
	}
	
	sensor *sptr = sensorListLookup( "VENT", id, 0, 0, type, batt );
	if ( sptr ) {
		if ( type & TEMPERATURE )
			sensorTemperature( sptr, temperature );
		if ( type & HUMIDITY)
			sensorHumidity( sptr, humidity );
		if ( type & RAINTOTAL)
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
			sensor *sptr = sensorListLookup( "MAND", id, channel, 0, type, batt );
			sensorWind( sptr, pri, 0.0, sec );
		} else if ( channel == 3 ) {
			type = RAINTOTAL;
			sensor *sptr = sensorListLookup( "MAND", id, channel, 0, type, batt );
			sensorRain( sptr, pri );
		}
	} else if ( sec ) {
		type = TEMPERATURE | HUMIDITY;
		sensor *sptr = sensorListLookup( "MAND", id, channel, 0, type, batt );
		sensorTemperature( sptr, pri );
		sensorHumidity( sptr, sec );
	} else {
		type = TEMPERATURE;
		sensor *sptr = sensorListLookup( "MAND", id, channel, 0, type, batt );
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
		type = RAINTOTAL;
		rain = (float) ( hex2char( s[6] ) * 100 + hex2char( s[7] ) * 10 + hex2char( s[9] ) ) * 0.03;
	} else {
		if ( humidity )
			type = TEMPERATURE | HUMIDITY;
		else
			type = TEMPERATURE;
	}
	
	// Temperature and humidity not tested!
	sensor *sptr = sensorListLookup( "FINE", id, 0, rolling, type, 1 );
	if ( sptr ) {
		if ( type & TEMPERATURE )
			sensorTemperature( sptr, temp );
		if ( type & HUMIDITY)
			sensorHumidity( sptr, humidity );
		if ( type & RAINTOTAL)
			sensorRain( sptr, rain );
	}
}

/**************************************************************************************************
 * WIRE
 * A custom library for sensors connected by wire to an arduino
 **************************************************************************************************/

void wired_parse( char *s ) {
	unsigned char id, value;
	SensorType    type;
	
	switch ( s[0] ) {
		case 'S':
			type  = SWITCH;
			id    = ( hex2char( s[1] ) << 4 ) | hex2char( s[2] );
			value = ( hex2char( s[3] ) << 4 ) | hex2char( s[4] );
			break;
		default:
			type = UNDEFINED;
	}
	
	sensor *sptr = sensorListLookup( "WIRE", id, 0, 0, type, 1 );
	if ( sptr ) {
		if ( type & SWITCH )
			sensorSwitch( sptr, value );
	}
}

