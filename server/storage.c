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

#include <stdio.h>
#include <stdlib.h>
#include <mysql.h>
#include <string.h>
#include "storage.h"
#include "config.h"

static const char * CREATE_TABLE_MYSQL[] =  {
#if _DEBUG > 1
	"DROP TABLE IF EXISTS weather_sensors ",
	"DROP TABLE IF EXISTS weather_rain ",
	"DROP TABLE IF EXISTS weather_temperature",
	"DROP TABLE IF EXISTS weather_humidity",
	"DROP TABLE IF EXISTS weather_wind",
#endif
	"CREATE TABLE IF NOT EXISTS weather_sensors( rowid INT NOT NULL AUTO_INCREMENT, name VARCHAR(64), protocol CHAR(4), channel TINYINT, code CHAR(2), battery TINYINT, type SMALLINT, PRIMARY KEY (rowid) )",
	"CREATE TABLE IF NOT EXISTS weather_rain( rowid INT NOT NULL AUTO_INCREMENT, created TIMESTAMP, amount FLOAT(10,2), PRIMARY KEY (rowid) )",
	"CREATE TABLE IF NOT EXISTS weather_temperature( rowid INT NOT NULL AUTO_INCREMENT, created TIMESTAMP, amount FLOAT(4,1), PRIMARY KEY (rowid) )",
	"CREATE TABLE IF NOT EXISTS weather_humidity( rowid INT NOT NULL AUTO_INCREMENT, created TIMESTAMP, amount TINYINT, PRIMARY KEY (rowid) )",
	"CREATE TABLE IF NOT EXISTS weather_wind( rowid INT NOT NULL AUTO_INCREMENT, created TIMESTAMP, speed DECIMAL(3,1), gust DECIMAL(3,1), direction SMALLINT, samples INT, PRIMARY KEY (rowid) );"
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
#if _DEBUG > 1
		for ( i=0; i<10; i++ ) {
#else
		for ( i=0; i<5; i++ ) {
#endif
			;
			if ( mysql_query( mysql, CREATE_TABLE_MYSQL[i] ) ) {
				fprintf( stderr, "ERROR: Could not create database table! Error Msg: %s.\n%s\n", mysql_error( mysql ), CREATE_TABLE_MYSQL[i] );
				mysql_close( mysql );
				exit( 7 );
			}
		}
	}
	
}
