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
#include <limits.h>			// INT_MIN / INT_MAX
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
	SWITCH      = 64,
	BAROMETER   = 128,
	DISTANCE    = 256,
	LEVEL       = 512,
	TEST        = 1024
} SensorType;

typedef struct {		//							Temp	Humid	Pellet	Baro
	double  r;          // Sensor noise variance	 0.2	 2.0	 10		 2
	double  pn;         // Process noise variance	 0.01	 0.05	 0.05	 0.25
	double  p;          // Predicted error
	double  x;          // Predicted value
	
	time_t  save_t;     // Next save time
	int     db_row;     // Database row
	int     save_i;     // Saved int value
	float   save_f;     // Saved float value
	int     type;       // Type of data
} DataStore;

typedef struct {
	float   save_f;     // Saved float value
	time_t  save_t;     // Next save time
	int     db_row;     // Database row
} DataFloat;

typedef struct {
	int     save_i;     // Saved float value
	time_t  save_t;     // Next save time
	int     db_row;     // Database row
} DataInt;

struct DataSample {
	float              speed;	// Wind speed
	float              gust;	// Wind gust
	short              dir;		// Wind direction
	time_t             time;	// Sample time
	struct DataSample *link;	// Next sample
};

typedef struct {
	float              x;			// Wind vector X
	float              y;			// Wind vector Y
	time_t             save_time;	// Next save time
	time_t             next_tx;		// Time to new transmission
	unsigned int       rowid;		// Database row
	struct DataSample *head;		// Fisrst stored value
	struct DataSample *tail;		// Last stored value
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
	DataStore		*temperature;
	DataStore		*humidity;
	DataFloat		*distance;
	DataStore		*level;
	DataStore		*barometer;
	DataInt  		*sw;
	DataStore		*test;
	DataFloat		*rain;
	DataWind		*wind;
} sensor;

sensor  *sensor_list;
MYSQL   *mysql;

char sensorInit();
void sensorMysqlInit();

// Sensor List
int sensorListInit();
sensor *sensorListLookup( const char *protocol, unsigned int sensor_id, unsigned char channel, unsigned char rolling, SensorType type, unsigned char battery  );
void sensorListFree();
sensor *sensorListAdd( unsigned int rowid, const char *name, const char *protocol, 
		unsigned int sensor_id, unsigned char channel, unsigned char rolling, 
		unsigned char battery, SensorType type );
sensor *sensorDbAdd( const char *protocol, unsigned int sensor_id, unsigned char channel, unsigned char rolling, SensorType type, unsigned char battery );
sensor *sensorDbSearch( const char *protocol, unsigned int sensor_id, unsigned char channel, unsigned char rolling, unsigned char battery );

char sensorMysqlInsert( sensor *s );

// Sensor update
char sensorUpdateBattery( sensor *s, unsigned char battery );
char sensorUpdateType( sensor *s, SensorType type );

// Wind sensor
DataWind *sensorWindInit();
void sensorWindDataInit( sensor *s );
char sensorWind( sensor *s, float speed, float gust, int dir );

// Sensor storage
char sensorTemperature( sensor *s, float value );
char sensorHumidity( sensor *s, float value );
char sensorRain( sensor *s, float total );
char sensorSwitch( sensor *s, char value );
char sensorDistance( sensor *s, float value );
char sensorLevel( sensor *s, float value );
char sensorBarometer( sensor *s, float value );
char sensorTest( sensor *s, float value );


// Data Initialization
DataStore *sensorDataInit( int type, double last_value, double m_noise, double p_noise );
DataInt   *sensorDataInt();
DataFloat *sensorDataFloat();

// Filters
float sensorKalmanFilter( DataStore *k, float value);
time_t sensorTimeSync();

#endif
