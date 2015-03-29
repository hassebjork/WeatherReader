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
#define M_PI          3.14159265358979323846  /* pi */
#define M_RAD         0.0174532925199
#define NEXT_TRANSMIT 15

extern ConfigSettings configFile;
unsigned int sensor_list_no = 0;

static const char * CREATE_TABLE_MYSQL[] =  {
#if _DEBUG > 4
// 	"DROP TABLE IF EXISTS wr_sensors",
// 	"DROP TABLE IF EXISTS wr_rain",
// 	"DROP TABLE IF EXISTS wr_temperature",
// 	"DROP TABLE IF EXISTS wr_humidity",
// 	"DROP TABLE IF EXISTS wr_wind",
// 	"DROP TABLE IF EXISTS wr_switch",
#endif
	"CREATE TABLE IF NOT EXISTS wr_sensors( id INT NOT NULL AUTO_INCREMENT, name VARCHAR(64) NOT NULL, sensor_id INT, protocol CHAR(4), channel TINYINT, rolling SMALLINT, battery TINYINT, team SMALLINT, color INT UNSIGNED, type SMALLINT, PRIMARY KEY (id) )",
	"CREATE TABLE IF NOT EXISTS wr_rain( id INT NOT NULL AUTO_INCREMENT, sensor_id INT, total FLOAT(10,2), time TIMESTAMP, PRIMARY KEY (id) )",
	"CREATE TABLE IF NOT EXISTS wr_temperature( id INT NOT NULL AUTO_INCREMENT, sensor_id INT, value FLOAT(4,1), time TIMESTAMP, PRIMARY KEY (id) )",
	"CREATE TABLE IF NOT EXISTS wr_humidity( id INT NOT NULL AUTO_INCREMENT, sensor_id INT, value TINYINT, time TIMESTAMP, PRIMARY KEY (id) )",
	"CREATE TABLE IF NOT EXISTS wr_wind( id INT NOT NULL AUTO_INCREMENT, sensor_id INT, speed DECIMAL(3,1), gust DECIMAL(3,1), dir SMALLINT, samples INT, time TIMESTAMP, PRIMARY KEY (id) );",
	"CREATE TABLE IF NOT EXISTS wr_switch( id INT NOT NULL AUTO_INCREMENT, sensor_id INT, value TINYINT, time TIMESTAMP, PRIMARY KEY (id) );",
	0
};

char sensorInit() {
	sensorListFree();
	sensor_list = (sensor *) malloc( sizeof( sensor ) );
	if ( !sensor_list ) {
		fprintf( stderr, "ERROR in sensorInit: Could not allocate memory for sensor_list\n" );
		return 1;
	}
	
	if ( configFile.mysql ) {
		sensorMysqlInit();
		return sensorListInit();
	}
	fprintf( stderr, "WARNING: No database configured! Sending raw data to stdout only!\n" );
	return 0;
}

void sensorMysqlInit() {
	int i;
	my_bool myb = 1;
	
	mysql = mysql_init( mysql );
	if ( !mysql ) {
		fprintf( stderr, "ERROR in sensorMysqlInit: Initiating MySQL database!\n%s\n", 
				 mysql_error( mysql ) );
		exit(5);
	}
	
	mysql_options( mysql, MYSQL_OPT_RECONNECT, &myb);
	if ( mysql_real_connect( mysql, configFile.mysqlServer, configFile.mysqlUser, configFile.mysqlPass, configFile.mysqlDatabase, 0, NULL, 0 ) == NULL ) {
		fprintf( stderr, "ERROR in sensorMysqlInit: Can not connect to MySQL database!\n%s\n", mysql_error( mysql ) );
		mysql_close( mysql );
		configFile.mysql = !configFile.mysql;
	} else {
		fprintf( stderr, "Using MySQL database:%*smysql://%s/%s\n", 9, "", configFile.mysqlServer, configFile.mysqlDatabase );
		
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

int sensorListInit() {
	MYSQL_RES   *result ;
	MYSQL_ROW    row;
	
	const char query[] = "SELECT id,name,protocol,sensor_id,channel,rolling,battery,type "
						 "FROM wr_sensors";
	if ( mysql_query( mysql, query ) ) {
		fprintf( stderr, "ERROR in sensorListInit: Selecting\n%s\n%s\n", mysql_error( mysql ), query );
		mysql_close( mysql );
		return 1;
	}
	
	result = mysql_store_result( mysql );
	if ( result == NULL ) {
		fprintf( stderr, "ERROR in sensorListInit: Storing result!\n%s\n", mysql_error( mysql ) );
		return 1;
	}
	
	while( ( row = mysql_fetch_row( result ) ) ) {
		sensorListAdd( atoi( row[0] ), row[1], row[2], atoi( row[3] ), atoi( row[4] ), 
					   atoi( row[5] ), atoi( row[6] ), atoi( row[7] ) );
	}
	return 0;
}

sensor *sensorListLookup( const char *protocol, unsigned int sensor_id, 
					  unsigned char channel, unsigned char rolling, 
					  SensorType type, unsigned char battery ) {
	sensor *ptr;
	int i;
	for ( i = 0; i < sensor_list_no; i++ ) {
		if ( sensor_list[i].sensor_id == sensor_id 
				&& sensor_list[i].channel == channel 
				&& sensor_list[i].rolling == rolling
				&& strncmp( protocol, sensor_list[i].protocol, 4 ) == 0 ) {
			
			// Update battery status if changed
			if ( sensor_list[i].battery != battery )
				sensorUpdateBattery( &sensor_list[i], battery );

			// Update type if incorrect
			if ( sensor_list[i].type^type )
				sensorUpdateType( &sensor_list[i], type );
			
			return &sensor_list[i];
		}
	}
	ptr = sensorDbSearch( protocol, sensor_id, channel, rolling, type, battery );
	if ( !ptr )
		return sensorDbAdd( protocol, sensor_id, channel, rolling, type, battery );
	return ptr;
}

void sensorListFree() {
	int i;
	for ( i = sensor_list_no - 1; i >= 0; i-- ) {
		if ( sensor_list[i].name != NULL )
			free( sensor_list[i].name );
		if ( sensor_list[i].temperature != NULL )
			free( sensor_list[i].temperature );
		if ( sensor_list[i].dataInt != NULL )
			free( sensor_list[i].dataInt );
		if ( sensor_list[i].rain != NULL )
			free( sensor_list[i].rain );
		if ( sensor_list[i].wind != NULL ) {
			struct DataSample *data;
			for( data = sensor_list[i].wind->head; data != NULL; data = data->link ) {
				free( sensor_list[i].wind->head );
				sensor_list[i].wind->head = data->link;
			}
			free( sensor_list[i].wind );
		}
	}
	free( sensor_list );
	sensor_list_no = 0;
}

sensor *sensorListAdd( unsigned int rowid, const char *name, const char *protocol, 
					   unsigned int sensor_id, unsigned char channel, unsigned char rolling, 
					   unsigned char battery, SensorType type ) {
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
	sensor_list[sensor_list_no].temperature = NULL;
	sensor_list[sensor_list_no].dataInt     = NULL;
	sensor_list[sensor_list_no].rain        = NULL;
	sensor_list[sensor_list_no].wind        = NULL;
	sensor_list_no++;
	return ptr;
}

sensor *sensorDbAdd( const char *protocol, unsigned int sensor_id, unsigned char channel, 
				   unsigned char rolling, SensorType type, unsigned char battery ) {
	if ( !configFile.sensorAutoAdd )
		return NULL;
	
	char query[256] = "";
	sensor *s = sensorListAdd( 0, "", protocol, sensor_id, channel, rolling, battery, type );
	
	sprintf( query, "INSERT INTO `wr_sensors`(`name`,`protocol`,`sensor_id`,`channel`,`rolling`,"
				    "`battery`,`type`) VALUES ('%s','%s',%d,%d,%d,%d,%d)", 
			 	    s->name, s->protocol, s->sensor_id, s->channel, s->rolling, 
				    s->battery, s->type );
	if ( mysql_query( mysql, query ) ) {
		fprintf( stderr, "ERROR in sensorMysqlInsert: Inserting\n%s\n%s\n", mysql_error( mysql ), query );
		return NULL;
	}
	s->rowid = mysql_insert_id( mysql );
	return s;
}

sensor *sensorDbSearch( const char *protocol, unsigned int sensor_id, unsigned char channel, 
					  unsigned char rolling, SensorType type, unsigned char battery ) {
	MYSQL_RES   *result ;
	MYSQL_ROW    row;
	char         query[256] = "";
	sensor      *s;
	
	sprintf( query, 
			"SELECT id,name,protocol,sensor_id,channel,rolling,battery,type "
			"FROM wr_sensors "
			"WHERE protocol='%s' AND sensor_id=%d AND channel=%d AND rolling=%d",
			protocol, sensor_id, channel, rolling );
	if ( mysql_query( mysql, query ) ) {
		fprintf( stderr, "ERROR in sensorDbSearch: Selecting\n%s\n%s\n", mysql_error( mysql ), query );
		return NULL;
	}
	
	result = mysql_store_result( mysql );
	if ( result == NULL ) {
		fprintf( stderr, "ERROR in sensorDbSearch: Storing result!\n%s\n", mysql_error( mysql ) );
		return NULL;
	}
	
	if( ( row = mysql_fetch_row( result ) ) )
		return sensorListAdd( atoi( row[0] ), row[1], row[2], atoi( row[3] ), atoi( row[4] ), 
							  atoi( row[5] ), atoi( row[6] ), atoi( row[7] ) );
	
	return NULL;
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

char sensorTemperature( sensor *s, float value ) {
#if _DEBUG > 2
	fprintf( stderr, "sensorTemperature: \t%s [row:%d (%s) id:%d] = %.1f\n", s->name, s->rowid, s->protocol, s->sensor_id, value );
#endif
	time_t now = sensorTimeSync();
	if ( s->temperature == NULL ) {
		s->temperature = (DataFloat *) malloc( sizeof( DataFloat ) );
		if ( !s->temperature ) {
			fprintf( stderr, "ERROR in sensorTemperature: Could not allocate memory for temperature\n" );
			return 1;
		}
		s->temperature->value  = -300.0;
		s->temperature->t_save = 0;
	}
	
	if ( s->temperature->value == value && ( configFile.saveTemperatureTime > 0 && now < s->temperature->t_save ) )
		return 0;
	
	// Save temperature
	char query[255] = "";
	sprintf( query, "INSERT INTO wr_temperature (sensor_id,value) "
					"VALUES(%d,%f)", s->rowid, value );
	if ( mysql_query( mysql, query ) ) {
		fprintf( stderr, "ERROR in sensorTemperature: Inserting\n%s\n%s\n", mysql_error( mysql ), query );
		return 1;
	}
	s->temperature->value  = value;
	s->temperature->t_save = (time_t) ( now / configFile.saveTemperatureTime + 1 ) * configFile.saveTemperatureTime;
	return 0;
}

char sensorHumidity( sensor *s, unsigned char value ) {
#if _DEBUG > 2
	fprintf( stderr, "sensorHumidity: \t%s [row:%d (%s) id:%d] = %d\n", s->name, s->rowid, s->protocol, s->sensor_id, value );
#endif
	time_t now = sensorTimeSync();
	
	if ( s->dataInt == NULL ) {
		s->dataInt = (DataInt *) malloc( sizeof( DataInt ) );
		if ( !s->dataInt ) {
			fprintf( stderr, "ERROR in sensorHumidity: Could not allocate memory for humidity\n" );
			return 1;
		}
		s->dataInt->value  = -1;
		s->dataInt->t_save = 0;
	}
	
	if ( s->dataInt->value == value 
			&& ( configFile.saveHumidityTime > 0 && now < s->dataInt->t_save ) )
		return 0;
	
	// Save humidity
	char query[255] = "";
	sprintf( query, "INSERT INTO wr_humidity (sensor_id,value) "
					"VALUES(%d,%d)", s->rowid, value );
	if ( mysql_query( mysql, query ) ) {
		fprintf( stderr, "ERROR in sensorHumidity: Inserting\n%s\n%s\n", mysql_error( mysql ), query );
		return 1;
	}
	s->dataInt->value  = value;
	s->dataInt->t_save = (time_t) ( now / configFile.saveHumidityTime + 1 ) * configFile.saveHumidityTime;
	return 0;
}

char sensorRain( sensor *s, float total ) {
#if _DEBUG > 2
	fprintf( stderr, "sensorRain: \t\t%s [row:%d (%s) id:%d] = %.1f\n", s->name, s->rowid, s->protocol, s->sensor_id, total );
#endif
	time_t now = sensorTimeSync();
	if ( s->rain == NULL ) {
		s->rain = (DataFloat *) malloc( sizeof( DataFloat ) );
		if ( !s->rain ) {
			fprintf( stderr, "ERROR in sensorRain: Could not allocate memory for rain\n" );
			return 1;
		}
		s->rain->value  = -1.0;
		s->rain->t_save = 0;
	}
	
	if ( s->rain->value == total && ( configFile.saveRainTime > 0 && now < s->rain->t_save ) )
		return 0;
	
	// Save rain
	char query[255] = "";
	sprintf( query, "INSERT INTO wr_rain (sensor_id,total) "
					"VALUES (%d,%f)", s->rowid, total );
	if ( mysql_query( mysql, query ) ) {
		fprintf( stderr, "ERROR in sensorRain: Inserting\n%s\n%s\n", mysql_error( mysql ), query );
		return 1;
	}
	s->rain->value  = total;
	s->rain->t_save = (time_t) ( now / configFile.saveRainTime + 1 ) * configFile.saveRainTime;
	return 0;
}

DataWind *sensorWindInit() {
	DataWind *wind = (DataWind *) malloc( sizeof( DataWind ) );
	if ( !wind ) {
		fprintf( stderr, "ERROR in sensorWind: Could not allocate memory for wind\n" );
		return NULL;
	}
	wind->save_time = 0;
	wind->next_tx   = 0;
	wind->x         = 0.0;
	wind->y         = 0.0;
	wind->rowid     = 0;
	wind->head      = NULL;
	wind->tail      = NULL;
	return wind;
}

void sensorWindDataInit( sensor *s ) {
	struct DataSample *data;
	// Reuse incomplete DataSample
	if ( s->wind->tail != NULL && s->wind->tail->time == 0 ) {
		data = s->wind->tail;
	
	// Create and queue DataSample
	} else {
		data = (struct DataSample *) malloc( sizeof( struct DataSample ) );
		if ( s->wind->tail == NULL ) {			// Create queue
			s->wind->tail = s->wind->head = data;
		} else {								// Add to queue if not added
			s->wind->tail->link = data;
			s->wind->tail = data;
		}
	}
	
	// Reset DataSample
	data->speed = -1.0;
	data->gust  = -1.0;
	data->dir   = -1;
	data->time  = 0;
	data->link  = NULL;
}

char sensorWind( sensor *s, float speed, float gust, int dir ) {
	time_t now = sensorTimeSync();
	struct DataSample *data;
	
#if _DEBUG > 2
	fprintf( stderr, "sensorWind:\tspeed:%0.1f\tgust:%0.1f\tdir:%d\n", speed, gust, dir );
#endif
	// Initialize wind
	if ( s->wind == NULL )
		s->wind = sensorWindInit();
	
	// New data set
	if ( now > s->wind->next_tx ) {
		s->wind->next_tx = now + NEXT_TRANSMIT;
		sensorWindDataInit( s );
	}
	
	// Store values
	data        = s->wind->tail;
	data->speed = ( speed >= 0.0 ? speed : data->speed );
	data->gust  = ( gust  >= 0.0 ? gust  : data->gust  );
	data->dir   = ( dir   >= 0   ? dir   : data->dir   );
	
	// Values incomplete or allready stored
	if ( data->time > 0 || data->speed < 0.0 || data->gust < 0.0 || data->dir < 0 )
		return 0;
	
	// Calculate
	float radians;
	int   samples = 0;
	data->time    = now;
	speed         = 0.0;
	if ( data->speed > 0.0 ) {
		radians     = M_RAD * data->dir;
		s->wind->x -= data->speed * sin( radians );
		s->wind->y -= data->speed * cos( radians );
	}
	
	// Remove old values
	for( data = s->wind->head; data != NULL && data->time < ( now - configFile.sampleWindTime ); data = data->link ) {
#if _DEBUG > 3
		fprintf( stderr, "\tdelete:\t[%d]\tspeed:%0.1f\tgust:%0.1f\tdir:%d\tlink:[%d]\n", data, data->speed, data->gust, data->dir, data->link );
#endif
		if ( data->speed > 0.0 ) {
			radians     = M_RAD * data->dir;
			s->wind->x += data->speed * sin( radians );
			s->wind->y += data->speed * cos( radians );
		}
		free( s->wind->head );
		s->wind->head = data->link;
	}
	
	// Current wind gust
	gust = -1.0;
	samples = 0;
	for( data = s->wind->head; data != NULL; data = data->link ) {
		samples++;
		speed += data->speed;
		if ( data->gust > gust )
			gust = data->gust;
#if _DEBUG > 3
		fprintf( stderr, "\t%*d.\t[%d]\tspeed:%0.1f\tgust:%0.1f\tdir:%d\tlink:[%d]\n", 5, samples, data, data->speed, data->gust, data->dir, data->link );
#endif
	}
	
	// Current avg wind speed
	speed = speed / samples;
	
	// Current wind direction
	if ( s->wind->x == 0 )
		dir = 0;
	else if ( s->wind->x > 0 )
		dir = (short)( 270 - 180 / M_PI * atan( s->wind->y / s->wind->x ) ) % 360;
	else
		dir = (short)(  90 - 180 / M_PI * atan( s->wind->y / s->wind->x ) ) % 360;
	
#if _DEBUG > 3
		fprintf( stderr, "\tstore:\tsamples:%d\tspeed:%0.1f\tgust:%0.1f\tdir:%d\ttime:%d\n", samples, speed, gust, dir, (s->wind->tail->time - s->wind->head->time) );
#endif
	// Store new data
	char query[255] = "";
	if ( now > s->wind->save_time ) {
		sprintf( query, "INSERT INTO wr_wind (sensor_id,speed,gust,dir,samples) "
						"VALUES (%d,%.1f,%.1f,%d,1)", 
						s->rowid, speed, gust, dir);
		
	// Update latest data to database
	} else if ( speed < 0.1 ) {
		sprintf( query, "UPDATE wr_wind SET speed=0,gust=%.1f,dir=NULL,"
						"samples=%d, time=NOW() WHERE id=%d", 
						gust, samples, s->wind->rowid );
	} else {
		sprintf( query, "UPDATE wr_wind SET speed=%.1f,gust=%.1f,dir=%d,"
						"samples=%d, time=NOW() WHERE id=%d", 
						speed, gust, dir, samples, s->wind->rowid );
	}
	
	if ( mysql_query( mysql, query ) ) {
		fprintf( stderr, "ERROR in sensorWind: %s\n%s\n", mysql_error( mysql ), query );
		return 1;
	} else if ( now > s->wind->save_time ) {
		s->wind->rowid = mysql_insert_id( mysql );
		s->wind->save_time = (time_t) ( now / configFile.sampleWindTime + 1 ) * configFile.sampleWindTime;
	}
	return 0;
}

char sensorSwitch( sensor *s, char value ) {
#if _DEBUG > 2
	fprintf( stderr, "sensorSwitch: \t%s [row:%d (%s) id:%d] = %d\n", s->name, s->rowid, s->protocol, s->sensor_id, value );
#endif
	time_t now = sensorTimeSync();
	
	if ( s->dataInt== NULL ) {
		s->dataInt = (DataInt *) malloc( sizeof( DataInt ) );
		if ( !s->dataInt ) {
			fprintf( stderr, "ERROR in sensorSwitch: Could not allocate memory for value\n" );
			return 1;
		}
		s->dataInt->value  = -1;
		s->dataInt->t_save = 0;
	}
	
	if ( s->dataInt->value == value )
		return 0;

	char query[255] = "";
	sprintf( query, "INSERT INTO wr_switch (sensor_id, value) "
					"VALUES(%d,%d)", s->rowid, value );
	if ( mysql_query( mysql, query ) ) {
		fprintf( stderr, "ERROR in sensorSwitch: Inserting\n%s\n%s\n", mysql_error( mysql ), query );
		return 1;
	}
	s->dataInt->value  = value;
	s->dataInt->t_save = (time_t) ( now / configFile.saveHumidityTime + 1 ) * configFile.saveHumidityTime;
	return 0;
}

time_t sensorTimeSync() {
	static time_t update = 0;
	static int    correction = 0, syncTime = 3600;
	time_t        now = time( NULL );
	
	if ( now < update )
		return (time_t) now - correction;
	
	// Sync time with database
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
	
	// Adjust sync time and add 10 sec so it will not become static
	diff = correction - diff;
	if ( diff < 0 )
		diff = -diff;
	if ( diff != 0 && update != 0 )
		syncTime = syncTime / diff;
	syncTime += 10;
	update = (time_t) ( now + syncTime - correction );
#if _DEBUG > 0
	struct tm *local = localtime( &now );
	struct tm *upd   = localtime( &update );
	fprintf( stderr, "[%02i:%02i:%02i] SyncTime:%*sCorrection: %+d sec\tNext update: %02i:%02i:%02i\n", 
			local->tm_hour, local->tm_min, local->tm_sec, 10, "", -correction,
			upd->tm_hour, upd->tm_min, upd->tm_sec );
#endif
	return (time_t) now - correction;
}
