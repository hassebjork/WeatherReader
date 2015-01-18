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
	RAIN        = 32
} SensorType;

typedef struct {
	float  value;
	time_t time;
} DataFloat;

typedef struct {
	int    value;
	time_t time;
} DataInt;

typedef struct {
	float			 speed;
	float			 gust;
	short			 dir;
	char			 saved;
	time_t			 time;
	float			 gust_max;
	float			*s_speed;
	short			*s_dir;
	time_t			 s_time;
	unsigned int 	 samples;
	unsigned int 	 row;
} DataWind;

typedef struct {
	unsigned int     rowid;
	char            *name;
	unsigned int     sensor_id;
	char    		 protocol[5];
	unsigned char    channel;
	unsigned char    rolling;
	unsigned char    battery;
	unsigned int     server;
	unsigned int     receiveRow;
	unsigned int     receiveCount;
	SensorType       type;
	DataFloat		*temperature;
	DataInt			*humidity;
	DataFloat		*rain;
	DataWind		*wind;
} sensor;

sensor  *sensor_list;
MYSQL   *mysql;

char sensorInit();
void sensorMysqlInit();
sensor *sensorSearch( const char *protocol, unsigned int sensor_id, unsigned char channel, unsigned char rolling, SensorType type, unsigned char battery );
sensor *sensorListAdd( unsigned int rowid, const char *name, const char *protocol, 
		unsigned int sensor_id, unsigned char channel, unsigned char rolling, 
		unsigned char battery, unsigned int server, SensorType type );
sensor *sensorAdd( const char *protocol, unsigned int sensor_id, unsigned char channel, unsigned char rolling, SensorType type, unsigned char battery );
char sensorMysqlInsert( sensor *s );
char sensorUpdateBattery( sensor *s, unsigned char battery );
char sensorUpdateType( sensor *s, SensorType type );
void sensorSaveTests();
char sensorReceiveTest( sensor *s );

char sensorTemperature( sensor *s, float value );
char sensorHumidity( sensor *s, unsigned char value );
char sensorRain( sensor *s, float total );
char sensorWind( sensor *s, float speed, float gust, int dir );

sensor *sensorLookup( const char *protocol, unsigned int sensor_id, unsigned char channel, unsigned char rolling, SensorType type, unsigned char battery  );

void sensorListFree();
time_t sensorTimeSync();
void sensorPrint( sensor *s );
void printTime( char *s );

#endif
