/*************************************************************************
   config.h

   This module reads a config file and stores the data, to be used in 
   the Weather Station reader program auriol-reader.
   
   This file was inspired by the rtl-wx project at GitHub
   https://github.com/magellannh/rtl-wx
   
   This file is part of the Weather Station reader program auriol-reader.
   
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

#ifndef _CONFIG_h
#define _CONFIG_h

#include <string.h>
#include <stdio.h>

#define CONFIG_FILE_NAME "/etc/weather-reader.conf"
#define READ_BUFSIZE 1000
#define MAX_TAG_SIZE 200

typedef struct {
	int   run;
	char  serialDevice[32];
	int   sensorAutoAdd;
	
	int   is_client;
	char  serverAddress[16];
	int   serverPort;
	int   is_server;
	int   listenPort;
	
	char  mysql;
	char  mysqlServer[MAX_TAG_SIZE];
	char  mysqlUser[MAX_TAG_SIZE];
	char  mysqlPass[MAX_TAG_SIZE];
	int   mysqlPort;
	char  mysqlDatabase[MAX_TAG_SIZE];
	
	int   saveTemperatureTime;
	int   saveHumidityTime;
	int   saveRainTime;
	int   sampleWindTime;
	
} ConfigSettings;

ConfigSettings configFile;

int confReadFile( char *inFname, ConfigSettings *conf );
int confStringVar( char *buf, char *matchStr, char *destStr );
int confIntVar( char *buf, char *matchStr, int *valp );
int confFloatVar( char *buf, char *matchStr, float *valp );

#endif
