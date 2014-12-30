/*************************************************************************
   storage.h

   This module stores weather data into a database. Either the file based
   server SQLite3 can be used or MySQL, wich gives you the option of 
   saving data on a remote host. Both servers can be used independantly.
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

#include "storage.h"

static const char * CREATE_TABLE_MYSQL[] =  {
#if _DEBUG > 1
#define CREATE_MYSQL_TABLE_NO 10
	"DROP TABLE IF EXISTS weather_sensors ",
	"DROP TABLE IF EXISTS weather_rain ",
	"DROP TABLE IF EXISTS weather_temperature",
	"DROP TABLE IF EXISTS weather_humidity",
	"DROP TABLE IF EXISTS weather_wind",
#else
#define CREATE_MYSQL_TABLE_NO 5
#endif
	"CREATE TABLE IF NOT EXISTS weather_sensors( id INT NOT NULL AUTO_INCREMENT, name VARCHAR(64) NOT NULL, sensor_id INT, protocol CHAR(4), channel TINYINT, rolling SMALLINT, battery TINYINT, type SMALLINT, PRIMARY KEY (id) )",
	"CREATE TABLE IF NOT EXISTS weather_rain( id INT NOT NULL AUTO_INCREMENT, sensor_id INT, amount FLOAT(10,2), time TIMESTAMP, PRIMARY KEY (id) )",
	"CREATE TABLE IF NOT EXISTS weather_temperature( id INT NOT NULL AUTO_INCREMENT, sensor_id INT, amount FLOAT(4,1), time TIMESTAMP, PRIMARY KEY (id) )",
	"CREATE TABLE IF NOT EXISTS weather_humidity( id INT NOT NULL AUTO_INCREMENT, sensor_id INT, amount TINYINT, time TIMESTAMP, PRIMARY KEY (id) )",
	"CREATE TABLE IF NOT EXISTS weather_wind( id INT NOT NULL AUTO_INCREMENT, sensor_id INT, speed DECIMAL(3,1), gust DECIMAL(3,1), direction SMALLINT, samples INT, time TIMESTAMP, PRIMARY KEY (id) );"
};

MYSQL   *mysql;
int error = 0;
struct tm *local;
time_t t, start_time;

extern ConfigSettings configFile;

void storageInit() {
	
	start_time = time( NULL );
	
	if ( configFile.mysql )
		storageMysqlInit();
	
	if ( !configFile.mysql )
		printf( "WARNING: No database configured! Sending raw data to stdout only!\n" );
}

void storageMysqlInit() {
	int i;
	mysql = mysql_init( NULL );
	
	if ( !mysql ) {
		fprintf( stderr, "ERROR when initiating MySQL database: %s\n", mysql_error( mysql ) );
		exit(5);
	}
	
	if ( mysql_real_connect( mysql, configFile.mysqlServer, configFile.mysqlUser, configFile.mysqlPass, configFile.mysqlDatabase, 0, NULL, 0 ) == NULL ) {
		fprintf( stderr, "ERROR when connecting to MySQL database: %s\n", mysql_error( mysql ) );
		mysql_close( mysql );
		configFile.mysql = !configFile.mysql;
	} else {
		printf( "Using MySQL database:\t\"mysql://%s/%s\"\n", configFile.mysqlServer, configFile.mysqlDatabase );
		
		/* Create database tables, if not exits */
		for ( i=0; i<CREATE_MYSQL_TABLE_NO; i++ ) {
			if ( mysql_query( mysql, CREATE_TABLE_MYSQL[i] ) ) {
				fprintf( stderr, "ERROR: Could not create database table! Error Msg: %s.\n%s\n", mysql_error( mysql ), CREATE_TABLE_MYSQL[i] );
				mysql_close( mysql );
				exit( 7 );
			}
		}
#if _DEBUG > 4
		for ( i=0; i<INSERT_MYSQL_SENSORS_NO; i++ ) {
			if ( mysql_query( mysql, INSERT_TABLE_MYSQL[i] ) ) {
				fprintf( stderr, "ERROR: Could not create database table! Error Msg: %s.\n%s\n", mysql_error( mysql ), CREATE_TABLE_MYSQL[i] );
				mysql_close( mysql );
				exit( 7 );
			}
		}
#endif
	}
}
