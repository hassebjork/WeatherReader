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

#include "sensor.h"

extern MYSQL   *mysql;
int sensor_list_no = 0;

sensor *sensorAdd( const char *protocol, unsigned int sensor_id, unsigned char channel, unsigned char rolling, int type, char battery ) {
	// http://stackoverflow.com/a/6170469/4405465
	sensor *ptr = (sensor *) realloc( sensor_list, (sensor_list_no + 1) * sizeof( sensor ) );
	if ( !ptr ) {
		fprintf( stderr, "sensorAdd - Out of memory allocating sensor_list\n" );
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
	char query[512] = "";
	sprintf( query, "INSERT INTO weather_sensors(name,protocol,sensor_id,channel,rolling,battery,type) VALUES ('%s','%s',%d,%d,%d,%d,%d)", 
			 s->name, s->protocol, s->sensor_id, s->channel, s->rolling, s->battery, s->type );
	if ( mysql_query( mysql, query ) ) {
		fprintf( stderr, "sensorMysqlInsert - Insert: %s\n%s\n", mysql_error( mysql ), query );
		return 1;
	}
	s->rowid = mysql_insert_id( mysql );
#if _DEBUG > 4
	printf( "SQL: %s\n", query );
#endif
	return 0;
}

sensor *sensorLookup( const char *protocol, unsigned int sensor_id, unsigned char channel, unsigned char rolling, int type  ) {
	sensor *ptr;
	int i;
	for ( i = 0; i < sensor_list_no; i++ ) {
		if ( sensor_list[i].sensor_id == sensor_id
				&& sensor_list[i].channel == channel
				&& sensor_list[i].type    == type
				&& sensor_list[i].rolling == rolling ) {
#if _DEBUG > 4
			printf( "Found " );
			sensorPrint( &sensor_list[i] );
#endif
			return &sensor_list[i];
		}
	}
	return NULL;
}

void sensorPrint( sensor *s ) {
	if ( !s ) return;
	printf( "MySQL-id:%i Name:%s\tSensor:%X Protocol:%s Channel:%d Rolling:%X Battery:%d Type:%d\n", 
			s->rowid, s->name, s->sensor_id, s->protocol, s->channel, s->rolling, s->battery, s->type );
}
