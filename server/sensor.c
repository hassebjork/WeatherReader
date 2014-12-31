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

extern ConfigSettings configFile;
int sensor_list_no = 0;

static const char * CREATE_TABLE_MYSQL[] =  {
#if _DEBUG > 4
	"DROP TABLE IF EXISTS wr_sensors ",
	"DROP TABLE IF EXISTS wr_rain ",
	"DROP TABLE IF EXISTS wr_temperature",
	"DROP TABLE IF EXISTS wr_humidity",
	"DROP TABLE IF EXISTS wr_wind",
#endif
	"CREATE TABLE IF NOT EXISTS wr_sensors( id INT NOT NULL AUTO_INCREMENT, name VARCHAR(64) NOT NULL, sensor_id INT, protocol CHAR(4), channel TINYINT, rolling SMALLINT, battery TINYINT, type SMALLINT, PRIMARY KEY (id) )",
	"CREATE TABLE IF NOT EXISTS wr_rain( id INT NOT NULL AUTO_INCREMENT, sensor_id INT, amount FLOAT(10,2), time TIMESTAMP, PRIMARY KEY (id) )",
	"CREATE TABLE IF NOT EXISTS wr_temperature( id INT NOT NULL AUTO_INCREMENT, sensor_id INT, amount FLOAT(4,1), time TIMESTAMP, PRIMARY KEY (id) )",
	"CREATE TABLE IF NOT EXISTS wr_humidity( id INT NOT NULL AUTO_INCREMENT, sensor_id INT, amount TINYINT, time TIMESTAMP, PRIMARY KEY (id) )",
	"CREATE TABLE IF NOT EXISTS wr_wind( id INT NOT NULL AUTO_INCREMENT, sensor_id INT, speed DECIMAL(3,1), gust DECIMAL(3,1), direction SMALLINT, samples INT, time TIMESTAMP, PRIMARY KEY (id) );",
	0
};

char sensorInit() {
	MYSQL_RES   *result ;
	MYSQL_ROW    row;
	
	if ( configFile.mysql )
		sensorMysqlInit();
	
	if ( !configFile.mysql )
		printf( "WARNING: No database configured! Sending raw data to stdout only!\n" );
	
	const char query[] = "SELECT id,name,protocol,sensor_id,channel,rolling,battery,type FROM wr_sensors";
	
	if ( mysql_query( mysql, query ) ) {
		fprintf( stderr, "ERROR in sensorMysqlFetchList: Selecting\n%s\n%s\n", mysql_error( mysql ), query );
		mysql_close( mysql );
		return 1;
	}
	
	result = mysql_store_result( mysql );
	if ( result == NULL ) {
		fprintf( stderr, "ERROR in sensorMysqlFetchList: Storing result!\n%s\n", mysql_error( mysql ) );
		return 1;
	}
	
	sensorListFree();
	sensor_list = (sensor *) malloc( sizeof( sensor ) * mysql_num_rows( result ) );
	if ( !sensor_list ) {
		fprintf( stderr, "ERROR in sensorMysqlFetchList: Could not allocate memory for sensor_list\n" );
		return 1;
	}
	
	while( ( row = mysql_fetch_row( result ) ) ) {
		sensor_list[sensor_list_no].rowid       = atoi( row[0] );
		sensor_list[sensor_list_no].name        = strdup( row[1] );
		strncpy( sensor_list[sensor_list_no].protocol, row[2], 4 );
		sensor_list[sensor_list_no].protocol[4] = '\0';
		sensor_list[sensor_list_no].sensor_id   = atoi( row[3] );
		sensor_list[sensor_list_no].channel     = atoi( row[4] );
		sensor_list[sensor_list_no].rolling     = atoi( row[5] );
		sensor_list[sensor_list_no].battery     = atoi( row[6] );
		sensor_list[sensor_list_no].type        = atoi( row[7] );
		sensor_list_no++;
	}
	return 0;
}

void sensorMysqlInit() {
	int i;
	mysql = mysql_init( NULL );
	
	if ( !mysql ) {
		fprintf( stderr, "ERROR in sensorMysqlInit: Initiating MySQL database!\n%s\n", mysql_error( mysql ) );
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

sensor *sensorAdd( const char *protocol, unsigned int sensor_id, unsigned char channel, unsigned char rolling, unsigned int type, unsigned char battery ) {
	// http://stackoverflow.com/a/6170469/4405465
	sensor *ptr = (sensor *) realloc( sensor_list, (sensor_list_no + 1) * sizeof( sensor ) );
	if ( !ptr ) {
		fprintf( stderr, "ERROR in sensorAdd: Out of memory allocating sensor_list\n" );
		return NULL;
	}
	sensor_list = ptr;
	
	sensor_list[sensor_list_no].sensor_id   = sensor_id;
	sensor_list[sensor_list_no].channel     = channel;
	sensor_list[sensor_list_no].rolling     = rolling;
	sensor_list[sensor_list_no].battery     = battery;
	sensor_list[sensor_list_no].type        = type;
 	strncpy( sensor_list[sensor_list_no].protocol, protocol, 4 );
 	sensor_list[sensor_list_no].protocol[4] = '\0';
 	sensor_list[sensor_list_no].name        = "";
 	sensor_list[sensor_list_no].rowid       = sensor_list_no;
	sensorMysqlInsert( &sensor_list[sensor_list_no] );
#if _DEBUG > 4
	printf( "Adding " );
	sensorPrint( &sensor_list[sensor_list_no] );
#endif
	++sensor_list_no;
	return &sensor_list[sensor_list_no];
}

char sensorMysqlInsert( sensor *s ) {
	char query[256] = "";
	sprintf( query, "INSERT INTO wr_sensors(name,protocol,sensor_id,channel,rolling,battery,type) VALUES ('%s','%s',%d,%d,%d,%d,%d)", 
			 s->name, s->protocol, s->sensor_id, s->channel, s->rolling, s->battery, s->type );
	if ( mysql_query( mysql, query ) ) {
		fprintf( stderr, "ERROR in sensorMysqlInsert: Inserting\n%s\n%s\n", mysql_error( mysql ), query );
		return 1;
	}
	s->rowid = mysql_insert_id( mysql );
#if _DEBUG > 4
	printf( "SQL: %s\n", query );
#endif
	return 0;
}

char sensorUpdateBattery( sensor *s ) {
	char query[128] = "";
	sprintf( query, "UPDATE wr_sensors SET battery=%d WHERE id=%d)", s->battery, s->rowid );
	if ( mysql_query( mysql, query ) ) {
		fprintf( stderr, "ERROR in sensorUpdateBattery: Updating\n%s\n%s\n", mysql_error( mysql ), query );
		return 1;
	}
	return 0;
}

sensor *sensorLookup( const char *protocol, unsigned int sensor_id, unsigned char channel, unsigned char rolling, unsigned int type, unsigned char battery ) {
	sensor *ptr;
	int i;
	for ( i = 0; i < sensor_list_no; i++ ) {
		if ( sensor_list[i].sensor_id == sensor_id
				&& sensor_list[i].channel == channel
				&& sensor_list[i].type    == type
				&& sensor_list[i].rolling == rolling ) {
			// Update sensor battery status if needed
			if ( sensor_list[i].battery != battery ) {
				sensor_list[i].battery == battery;
				sensorUpdateBattery( &sensor_list[i] );
			}
#if _DEBUG > 4
			printf( "Found " );
			sensorPrint( &sensor_list[i] );
#endif
			return &sensor_list[i];
		}
	}
	return sensorAdd( protocol, sensor_id, channel, rolling, type, battery );
}

void sensorListFree() {
	int i;
	for ( i = sensor_list_no - 1; i >= 0; i-- )
		free( sensor_list[i].name );
	free( sensor_list );
	sensor_list_no = 0;
}

void sensorPrint( sensor *s ) {
	if ( !s ) return;
	printf( "id:%i Name:%s\tSensor:%X Protocol:%s Channel:%d Rolling:%X Battery:%d Type:%d\n", 
			s->rowid, s->name, s->sensor_id, s->protocol, s->channel, s->rolling, s->battery, s->type );
}
