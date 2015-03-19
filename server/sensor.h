/*************************************************************************
   sensor.h

   This module stores data of weather sensors into a MySQL database.
   
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

#ifndef _SENSOR_H_
#define _SENSOR_H_

#include <stdio.h>
#include <stdlib.h>
#include <mysql.h>
#include <string.h>
#include <time.h>
#include <float.h>
#include <math.h>
#include "config.h"

typedef enum {
	UNDEFINED   = 0,
	TEMPERATURE = 1,
	HUMIDITY    = 2,
	WINDSPEED   = 4,
	WINDGUST    = 8,
	WINDDIR  	= 16,
	RAINTOTAL   = 32,
	SWITCH      = 64
} SensorType;

typedef struct {
	float  value;				// Float data
	time_t t_save;				// Next save time
} DataFloat;

typedef struct {
	int    value;				// Integer data
	time_t t_save;				// Next save time
} DataInt;

typedef struct {
	float			 speed;		// Current wind speed
	float			 gust;		// Current gust speed
	short			 dir;		// Current wind direction
	char			 saved;		// Data saved to database
	float			 gust_max;	// Max gust within this save time
	float			*s_speed;	// Array of Wind speed data
	short			*s_dir;		// Array of Wind direction
	unsigned int 	 samples;	// No. of samples in s_dir and s_speed
	time_t			 s_time;	// Next save time
	time_t			 t_trans;	// Time to new transmission
	unsigned int 	 rowid;		// Database row
} DataWind;

typedef struct {
	unsigned int     rowid;		// Database row
	char            *name;		// Name of sensor
	unsigned int     sensor_id;	// Sensors own id
	char    		 protocol[5];	// Protocol
	unsigned char    channel;	// Sensor channel
	unsigned char    rolling;	// Random code
	unsigned char    battery;	// Sensor battery status Full = 1
	SensorType       type;		// Type of sensor
	DataFloat		*temperature;
	DataInt			*dataInt;
	DataFloat		*rain;
	DataWind		*wind;
} sensor;

sensor  *sensor_list;
MYSQL   *mysql;

char sensorInit();
void sensorMysqlInit();

int sensorListInit();
sensor *sensorListLookup( const char *protocol, unsigned int sensor_id, unsigned char channel, unsigned char rolling, SensorType type, unsigned char battery  );
void sensorListFree();
sensor *sensorListAdd( unsigned int rowid, const char *name, const char *protocol, 
		unsigned int sensor_id, unsigned char channel, unsigned char rolling, 
		unsigned char battery, SensorType type );

sensor *sensorDbSearch( const char *protocol, unsigned int sensor_id, unsigned char channel, unsigned char rolling, SensorType type, unsigned char battery );
sensor *sensorDbAdd( const char *protocol, unsigned int sensor_id, unsigned char channel, unsigned char rolling, SensorType type, unsigned char battery );
char sensorMysqlInsert( sensor *s );
char sensorUpdateBattery( sensor *s, unsigned char battery );
char sensorUpdateType( sensor *s, SensorType type );

char sensorTemperature( sensor *s, float value );
char sensorHumidity( sensor *s, unsigned char value );
char sensorRain( sensor *s, float total );
char sensorWind( sensor *s, float speed, float gust, int dir );
char sensorSwitch( sensor *s, char value );

time_t sensorTimeSync();
void sensorPrint( sensor *s );
void printTime( char *s );

#endif
