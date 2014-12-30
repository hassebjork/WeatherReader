/*************************************************************************
   sensor.h

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

#ifndef _SENSOR_H_
#define _SENSOR_H_

#include <stdio.h>
#include <stdlib.h>
#include <mysql.h>
#include <string.h>
#include "storage.h"

typedef struct {
	int     rowid;
	char   *name;
	int     sensor_id;
	char    protocol[5];
	char    channel;
	char    rolling;
	char    battery;
	int     type;
} sensor;

sensor  *sensor_list;

sensor *sensorAdd( const char *protocol, unsigned int sensor_id, unsigned char channel, unsigned char rolling, int type, char battery );
char sensorMysqlInsert( sensor *s );

sensor *sensorLookup( const char *protocol, unsigned int sensor_id, unsigned char channel, unsigned char rolling, int type );

void sensorPrint( sensor *s );

#endif
