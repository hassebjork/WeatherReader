/*************************************************************************
   sensor.c

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

#include "sensor.h"
#define M_PI         3.14159265358979323846  /* pi */

extern ConfigSettings configFile;
unsigned int sensor_list_no = 0;

static const char * CREATE_TABLE_MYSQL[] =  {
#if _DEBUG > 1
// 	"DROP TABLE IF EXISTS wr_sensors ",
	"DROP TABLE IF EXISTS wr_rain ",
	"DROP TABLE IF EXISTS wr_temperature",
	"DROP TABLE IF EXISTS wr_humidity",
	"DROP TABLE IF EXISTS wr_wind",
#endif
	"DROP TABLE IF EXISTS wr_test",
	"CREATE TABLE IF NOT EXISTS wr_sensors( id INT NOT NULL AUTO_INCREMENT, name VARCHAR(64) NOT NULL, sensor_id INT, protocol CHAR(4), channel TINYINT, rolling SMALLINT, battery TINYINT, server INT, team SMALLINT, type SMALLINT, PRIMARY KEY (id) )",
	"CREATE TABLE IF NOT EXISTS wr_rain( id INT NOT NULL AUTO_INCREMENT, sensor_id INT, total FLOAT(10,2), time TIMESTAMP, PRIMARY KEY (id) )",
	"CREATE TABLE IF NOT EXISTS wr_temperature( id INT NOT NULL AUTO_INCREMENT, sensor_id INT, value FLOAT(4,1), time TIMESTAMP, PRIMARY KEY (id) )",
	"CREATE TABLE IF NOT EXISTS wr_humidity( id INT NOT NULL AUTO_INCREMENT, sensor_id INT, value TINYINT, time TIMESTAMP, PRIMARY KEY (id) )",
	"CREATE TABLE IF NOT EXISTS wr_wind( id INT NOT NULL AUTO_INCREMENT, sensor_id INT, speed DECIMAL(3,1), gust DECIMAL(3,1), dir SMALLINT, samples INT, time TIMESTAMP, PRIMARY KEY (id) );",
	"CREATE TABLE IF NOT EXISTS wr_test( id INT NOT NULL AUTO_INCREMENT, sensor_id INT, server INT, count INT, time TIMESTAMP, PRIMARY KEY (id) );",
	0
};

char sensorInit() {
	MYSQL_RES   *result ;
	MYSQL_ROW    row;
	
	if ( configFile.mysql )
		sensorMysqlInit();
	
	if ( !configFile.mysql )
		printf( "WARNING: No database configured! Sending raw data to stdout only!\n" );
	
	const char query[] = "SELECT id,name,protocol,sensor_id,channel,rolling,battery,server,type "
						 "FROM wr_sensors";
	if ( mysql_query( mysql, query ) ) {
		fprintf( stderr, "ERROR in sensorInit: Selecting\n%s\n%s\n", mysql_error( mysql ), query );
		mysql_close( mysql );
		return 1;
	}
	
	result = mysql_store_result( mysql );
	if ( result == NULL ) {
		fprintf( stderr, "ERROR in sensorInit: Storing result!\n%s\n", mysql_error( mysql ) );
		return 1;
	}
	
	sensorListFree();
	sensor_list = (sensor *) malloc( sizeof( sensor ) * mysql_num_rows( result ) );
	if ( !sensor_list ) {
		fprintf( stderr, "ERROR in sensorInit: Could not allocate memory for sensor_list\n" );
		return 1;
	}
	
	while( ( row = mysql_fetch_row( result ) ) ) {
		sensorListAdd( atoi( row[0] ), row[1], row[2], atoi( row[3] ), atoi( row[4] ), 
					   atoi( row[5] ), atoi( row[6] ), atoi( row[7] ), atoi( row[8] ) );
	}
	return 0;
}

void sensorMysqlInit() {
	int i;
	mysql = mysql_init( NULL );
	
	if ( !mysql ) {
		fprintf( stderr, "ERROR in sensorMysqlInit: Initiating MySQL database!\n%s\n", 
				 mysql_error( mysql ) );
		exit(5);
	}
	
	if ( mysql_real_connect( mysql, configFile.mysqlServer, configFile.mysqlUser, configFile.mysqlPass, configFile.mysqlDatabase, 0, NULL, 0 ) == NULL ) {
		fprintf( stderr, "ERROR in sensorMysqlInit: Can not connect to MySQL database!\n%s\n", mysql_error( mysql ) );
		mysql_close( mysql );
		configFile.mysql = !configFile.mysql;
	} else {
		printf( "Using MySQL database:\t\"mysql://%s/%s\"\n", configFile.mysqlServer, configFile.mysqlDatabase );
		
		/* Create database tables, if not exits */
		for ( i=0; CREATE_TABLE_MYSQL[i] != 0; i++ ) {
			if ( mysql_query( mysql, CREATE_TABLE_MYSQL[i] ) ) {
				fprintf( stderr, "ERROR in sensorMysqlInit: Could not create db-table!\n%s.\n%s\n", mysql_error( mysql ), CREATE_TABLE_MYSQL[i] );
				mysql_close( mysql );
				exit( 7 );
			}
		}
	}
}

sensor *sensorSearch( const char *protocol, unsigned int sensor_id, unsigned char channel, 
					  unsigned char rolling, SensorType type, unsigned char battery ) {
	MYSQL_RES   *result ;
	MYSQL_ROW    row;
	char         query[256] = "";
	sensor      *s;
	
	sprintf( query, 
			"SELECT id,name,protocol,sensor_id,channel,rolling,battery,server,type "
			"FROM wr_sensors "
			"WHERE protocol='%s' AND sensor_id=%d AND channel=%d AND rolling=%d",
			protocol, sensor_id, channel, rolling );
	if ( mysql_query( mysql, query ) ) {
		fprintf( stderr, "ERROR in sensorSearch: Selecting\n%s\n%s\n", mysql_error( mysql ), query );
		return NULL;
	}
	
	result = mysql_store_result( mysql );
	if ( result == NULL ) {
		fprintf( stderr, "ERROR in sensorSearch: Storing result!\n%s\n", mysql_error( mysql ) );
		return NULL;
	}
	
	if( ( row = mysql_fetch_row( result ) ) )
		return sensorListAdd( atoi( row[0] ), row[1], row[2], atoi( row[3] ), atoi( row[4] ), 
							  atoi( row[5] ), atoi( row[6] ), atoi( row[7] ), atoi( row[8] ) );
	
	return NULL;
}

sensor *sensorListAdd( unsigned int rowid, const char *name, const char *protocol, 
					   unsigned int sensor_id, unsigned char channel, unsigned char rolling, 
					   unsigned char battery, unsigned int server, SensorType type ) {
	// http://stackoverflow.com/a/6170469/4405465
	sensor *ptr = (sensor *) realloc( sensor_list, (sensor_list_no + 1) * sizeof( sensor ) );
	if ( !ptr ) {
		fprintf( stderr, "ERROR in sensorListAdd: Out of memory allocating sensor_list\n" );
		return NULL;
	}
	sensor_list = ptr;
	ptr = &sensor_list[sensor_list_no];
	sensor_list[sensor_list_no].rowid       = rowid;
	sensor_list[sensor_list_no].name        = strdup( name );
	strncpy( sensor_list[sensor_list_no].protocol, protocol, 5 );
	sensor_list[sensor_list_no].protocol[4] = '\0';
	sensor_list[sensor_list_no].sensor_id   = sensor_id;
	sensor_list[sensor_list_no].channel     = channel;
	sensor_list[sensor_list_no].rolling     = rolling;
	sensor_list[sensor_list_no].battery     = battery;
	sensor_list[sensor_list_no].type        = type;
	sensor_list[sensor_list_no].server      = server;
	sensor_list[sensor_list_no].receiveRow  = 0;
	sensor_list[sensor_list_no].receiveCount= 0;
	sensor_list[sensor_list_no].temperature = NULL;
	sensor_list[sensor_list_no].humidity    = NULL;
	sensor_list[sensor_list_no].rain        = NULL;
	sensor_list[sensor_list_no].wind        = NULL;
	sensor_list_no++;
	return ptr;
}

sensor *sensorAdd( const char *protocol, unsigned int sensor_id, unsigned char channel, 
				   unsigned char rolling, SensorType type, unsigned char battery ) {
	if ( !configFile.sensorAutoAdd )
		return NULL;
	sensor *s = sensorListAdd( 0, "", protocol, sensor_id, channel, rolling, battery, 
							   0xFFFF, type );
	sensorMysqlInsert( s );
	return s;
}

char sensorMysqlInsert( sensor *s ) {
	char query[256] = "";
	sprintf( query, "INSERT INTO wr_sensors(name,protocol,sensor_id,channel,rolling,"
				    "battery,server,type) VALUES ('%s','%s',%d,%d,%d,%d,%d,%d)", 
			 	    s->name, s->protocol, s->sensor_id, s->channel, s->rolling, 
				    s->battery, s->server, s->type );
	if ( mysql_query( mysql, query ) ) {
		fprintf( stderr, "ERROR in sensorMysqlInsert: Inserting\n%s\n%s\n", mysql_error( mysql ), query );
		return 1;
	}
	s->rowid = mysql_insert_id( mysql );
	return 0;
}

char sensorUpdateBattery( sensor *s, unsigned char battery ) {
	char query[255] = "";
	sprintf( query, "UPDATE wr_sensors SET battery=%d WHERE id=%d", battery, s->rowid );
	if ( mysql_query( mysql, query ) ) {
		fprintf( stderr, "ERROR in sensorUpdateBattery: Updating\n%s\n%s\n", mysql_error( mysql ), query );
		return 1;
	}
	s->battery = battery;
	return 0;
}

char sensorUpdateType( sensor *s, SensorType type ) {
	char query[255] = "";
	sprintf( query, "UPDATE wr_sensors SET type=%d WHERE id=%d", ( s->type | type ), s->rowid );
	if ( mysql_query( mysql, query ) ) {
		fprintf( stderr, "ERROR in sensorUpdateType: Updating\n%s\n%s\n", mysql_error( mysql ), query );
		return 1;
	}
	s->type |= type;
	return 0;
}

void sensorSaveTests() {
	char query[255] = "";
	int i;
	MYSQL_RES   *result ;
	MYSQL_ROW    row;

	for ( i = 0; i < sensor_list_no; i++ ) {
		sprintf( query, "INSERT INTO wr_test (sensor_id,server,count) VALUES (%d,%d,%d)", 
				sensor_list[i].rowid, configFile.serverID, sensor_list[i].receiveCount );
		if ( sensor_list[i].receiveCount > 0 && mysql_query( mysql, query ) ) {
			fprintf( stderr, "ERROR in sensorReceiveTest: Inserting\n%s\n%s\n", 
						mysql_error( mysql ), query );
		}
		sensor_list[i].receiveCount = 0;
	}
}

char sensorReceiveTest( sensor *s ) {
	static time_t next = 0;
	time_t now = sensorTimeSync();
	
	if ( now > next ) {
		if ( next > 0 )
			sensorSaveTests();
		next = (time_t) ( now / 3600 + 1 ) * 3600;
	}
	++(s->receiveCount);
	return 0;
}

char sensorTemperature( sensor *s, float value ) {
	if ( configFile.serverID > 0 && !( s->server & configFile.serverID ) )
		return 0;
	time_t now = sensorTimeSync();
	if ( s->temperature == NULL ) {
		s->temperature = (DataFloat *) malloc( sizeof( DataFloat ) );
		if ( !s->temperature ) {
			fprintf( stderr, "ERROR in sensorTemperature: Could not allocate memory for temperature\n" );
			return 1;
		}
		s->temperature->value = -300.0;
		s->temperature->time   = 0;
	}
	
	if ( s->temperature->value == value && ( configFile.saveTemperatureTime > 0 && now < s->temperature->time ) )
		return 0;

	char query[255] = "";
	sprintf( query, "INSERT INTO wr_temperature (sensor_id, value,time) "
					"VALUES(%d,%f,FROM_UNIXTIME(%d))", s->rowid, value, now );
	if ( mysql_query( mysql, query ) ) {
		fprintf( stderr, "ERROR in sensorTemperature: Inserting\n%s\n%s\n", mysql_error( mysql ), query );
		return 1;
	}
	s->temperature->value = value;
	s->temperature->time  = (time_t) ( now / configFile.saveTemperatureTime + 1 ) 
							* configFile.saveTemperatureTime;
	return 0;
}

char sensorHumidity( sensor *s, unsigned char value ) {
	if ( configFile.serverID > 0 && !( s->server & configFile.serverID ) )
		return 0;
	time_t now = sensorTimeSync();
	
	if ( s->humidity == NULL ) {
		s->humidity = (DataInt *) malloc( sizeof( DataInt ) );
		if ( !s->humidity ) {
			fprintf( stderr, "ERROR in sensorHumidity: Could not allocate memory for humidity\n" );
			return 1;
		}
		s->humidity->value  = -1;
		s->humidity->time   = 0;
	}
	
	if ( s->humidity->value == value 
			&& ( configFile.saveHumidityTime > 0 && now < s->humidity->time ) )
		return 0;

	char query[255] = "";
	sprintf( query, "INSERT INTO wr_humidity (sensor_id, value, time) "
					"VALUES(%d,%d,FROM_UNIXTIME(%d))", s->rowid, value, now );
	if ( mysql_query( mysql, query ) ) {
		fprintf( stderr, "ERROR in sensorHumidity: Inserting\n%s\n%s\n", mysql_error( mysql ), query );
		return 1;
	}
	s->humidity->value = value;
	s->humidity->time  = (time_t) ( now / configFile.saveHumidityTime + 1 ) 
						* configFile.saveHumidityTime;
	return 0;
}

char sensorRain( sensor *s, float total ) {
	if ( configFile.serverID > 0 && !( s->server & configFile.serverID ) )
		return 0;
	time_t now = sensorTimeSync();
	if ( s->rain == NULL ) {
		s->rain = (DataFloat *) malloc( sizeof( DataFloat ) );
		if ( !s->rain ) {
			fprintf( stderr, "ERROR in sensorRain: Could not allocate memory for rain\n" );
			return 1;
		}
		s->rain->value = -1.0;
		s->rain->time   = 0;
	}
	
	if ( s->rain->value == total && ( configFile.saveRainTime > 0 && now < s->rain->time ) )
		return 0;

	char query[255] = "";
	sprintf( query, "INSERT INTO wr_rain (sensor_id, total,time) "
					"VALUES (%d,%f,FROM_UNIXTIME(%d))", s->rowid, total, now );
	if ( mysql_query( mysql, query ) ) {
		fprintf( stderr, "ERROR in sensorRain: Inserting\n%s\n%s\n", mysql_error( mysql ), query );
		return 1;
	}
	s->rain->value = total;
	s->rain->time  = (time_t) ( now / configFile.saveRainTime + 1 ) 
					 * configFile.saveRainTime;
	return 0;
}

char sensorWind( sensor *s, float speed, float gust, int dir ) {
	if ( configFile.serverID > 0 && !( s->server & configFile.serverID ) )
		return 0;
	time_t now = sensorTimeSync();
	
	// Initialize
	if ( s->wind == NULL ) {
		s->wind = (DataWind *) malloc( sizeof( DataWind ) );
		if ( !s->wind ) {
			fprintf( stderr, "ERROR in sensorWind: Could not allocate memory for wind\n" );
			return 1;
		}
		s->wind->time     = 0;
		s->wind->samples  = 0;
		s->wind->gust_max = 0.0;
		s->wind->s_time   = 0;
		s->wind->row      = 0;
		s->wind->s_speed  = (float *) malloc( sizeof( float ) );
		s->wind->s_dir    = (short *) malloc( sizeof( short ) );
	}
	
	// New data sent
	if ( now > s->wind->time ) {
		s->wind->speed = -1.0;
		s->wind->gust  = -1.0;
		s->wind->dir   = -1;
		s->wind->saved = 0;
		s->wind->time  = now + 10;
	}
	
	// Store values
	if ( speed >= 0.0 )
		s->wind->speed = speed;
	if ( gust >= 0.0 )
		s->wind->gust  = gust;
	if ( dir  >= 0 )
		s->wind->dir   = dir;
	
	// Values incomplete
	if ( s->wind->saved > 0 || s->wind->speed < 0.0 || s->wind->gust < 0.0 || s->wind->dir < 0 )
		return 0;
	
	// Store new data
	char query[255] = "";
	if ( now > s->wind->s_time ) {
		s->wind->gust_max = 0;
		s->wind->samples  = 0;
		s->wind->s_time   = (time_t) ( now / configFile.sampleWindTime + 1 ) 
							* configFile.sampleWindTime;
		sprintf( query, "INSERT INTO wr_wind (sensor_id,speed,gust,dir,samples,time) "
						"VALUES (%d,%.1f,%.1f,%d,1,FROM_UNIXTIME(%d))", 
						s->rowid, s->wind->speed, s->wind->gust, s->wind->dir, now );
		if ( mysql_query( mysql, query ) ) {
			fprintf( stderr, "ERROR in sensorRain: Inserting\n%s\n%s\n", mysql_error( mysql ), query );
			return 1;
		}
		s->wind->row = mysql_insert_id( mysql );
	}
	
	// Allocate memory
	s->wind->s_speed = (float *) realloc( s->wind->s_speed, 
									( s->wind->samples + 1 ) * sizeof( float ) );
	s->wind->s_dir   = (short *) realloc( s->wind->s_dir,   
									( s->wind->samples + 1 ) * sizeof( short ) );
	if ( !s->wind->s_speed || !s->wind->s_dir ) {
		fprintf( stderr, "ERROR in sensorWind: Out of memory allocating sample\n" );
		return 1;
	}
	
	// Save sample
	s->wind->s_speed[s->wind->samples] = s->wind->speed;
	s->wind->s_dir[s->wind->samples]   = s->wind->dir;
	if ( s->wind->gust > s->wind->gust_max )
		s->wind->gust_max = s->wind->gust;
	s->wind->samples++;
	s->wind->saved++;
	
	if ( s->wind->samples == 1 )
		return 0;
	
	// Calculate average
	int i;
	float x = 0.0, y = 0.0, rad;
	for ( i = s->wind->samples-1; i >= 0; i-- ) {
		rad = M_PI / 180 * s->wind->s_dir[i];
		x  += -s->wind->s_speed[i] * sin( rad );
		y  += -s->wind->s_speed[i] * cos( rad );
		speed += s->wind->s_speed[i];
	}
	speed = speed / s->wind->samples;
	x = x / s->wind->samples;
	y = y / s->wind->samples;
	if ( x == 0 )
		dir = 0;
	else if ( x > 0 )
		dir = 270 - 180 / M_PI * atan( y/x );
	else
		dir = 90  - 180 / M_PI * atan( y/x );
	dir = dir % 360;
	
	// Update latest data to database
	sprintf( query, "UPDATE wr_wind SET speed=%.1f,gust=%.1f,dir=%d,"
					"samples=%d, time=FROM_UNIXTIME(%d) WHERE id=%d", 
					speed, s->wind->gust_max, dir, s->wind->samples, 
					now, s->wind->row );
	if ( mysql_query( mysql, query ) ) {
		fprintf( stderr, "ERROR in sensorRain: Inserting\n%s\n%s\n", mysql_error( mysql ), query );
		return 1;
	}
	return 0;
}

sensor *sensorLookup( const char *protocol, unsigned int sensor_id, 
					  unsigned char channel, unsigned char rolling, 
					  SensorType type, unsigned char battery ) {
	sensor *ptr;
	int i;
	for ( i = 0; i < sensor_list_no; i++ ) {
		if ( sensor_list[i].sensor_id == sensor_id 
				&& sensor_list[i].channel == channel 
				&& sensor_list[i].rolling == rolling ) {
			
			// Update battery status if changed
			if ( sensor_list[i].battery != battery )
				sensorUpdateBattery( &sensor_list[i], battery );

			// Update type if incorrect
			if ( sensor_list[i].type^type )
				sensorUpdateType( &sensor_list[i], type );
			
			if ( configFile.sensorReceiveTest > 0 )
				sensorReceiveTest( &sensor_list[i] );

			return &sensor_list[i];
		}
	}
	ptr = sensorSearch( protocol, sensor_id, channel, rolling, type, battery );
	if ( !ptr )
		return sensorAdd( protocol, sensor_id, channel, rolling, type, battery );
	return ptr;
}

void sensorListFree() {
	int i, j;
	for ( i = sensor_list_no - 1; i >= 0; i-- ) {
		if ( sensor_list[i].name != NULL )
			free( sensor_list[i].name );
		if ( sensor_list[i].temperature != NULL )
			free( sensor_list[i].temperature );
		if ( sensor_list[i].humidity != NULL )
			free( sensor_list[i].humidity );
		if ( sensor_list[i].rain != NULL )
			free( sensor_list[i].rain );
		if ( sensor_list[i].wind != NULL ) {
			free( sensor_list[i].wind->s_speed );
			free( sensor_list[i].wind->s_dir   );
			free( sensor_list[i].wind );
		}
	}
	free( sensor_list );
	sensor_list_no = 0;
}

time_t sensorTimeSync() {
	static time_t update = 0;
	static int    correction = 0, syncTime = 3600;
	time_t        now = time( NULL );
	
	if ( now > update ) {
		char query[255] = "";
		int  diff = correction;
		MYSQL_RES   *result ;
		MYSQL_ROW    row;
		
		// Fetch database time and calculate correction
		sprintf( query, "SELECT UNIX_TIMESTAMP()" );
		mysql_query( mysql, query );
		result = mysql_store_result( mysql );
		if( result && ( row = mysql_fetch_row( result ) ) ) 
			correction = now - atoi( row[0] );
		else
			correction = 0;
		
		// Adjust sync time and add 1 sec so it will not become static
		diff = correction - diff;
		if ( diff < 0 )
			diff = -diff;
		if ( diff != 0 && update != 0 )
			syncTime = syncTime / diff;
		syncTime += 10;
#if _DEBUG > 1
		char s[20];
		printTime( s );
		fprintf( stderr, "%s SyncTime: %d\tCorr: %d\tDiff: %d\n", s, syncTime, correction, diff );
#endif
		update = (time_t) now + syncTime - correction;
	}
	return (time_t) now - correction;
}

void sensorPrint( sensor *s ) {
	if ( !s ) return;
	printf( "id:%i Name:%s\tSensor:%X Protocol:%s Channel:%d "
			"Rolling:%X Battery:%d Type:%d\n", 
			s->rowid, s->name, s->sensor_id, s->protocol, s->channel, 
			s->rolling, s->battery, s->type );
}

/**
 * printTime returns a formatted string of current time in s (min 20 characters)
 */
void printTime( char *s ) {
	struct tm *local;
	time_t t = time(NULL);
	local = localtime(&t);
	sprintf(s, "[%i-%02i-%02i %02i:%02i:%02i]", 
			(local->tm_year + 1900), 
			(local->tm_mon) + 1, 
			local->tm_mday, 
			local->tm_hour,
			local->tm_min, 
			local->tm_sec );
}
