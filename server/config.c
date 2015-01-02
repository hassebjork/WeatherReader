/*************************************************************************
   config.c

   This module reads a config file and stores the data, to be used in 
   the Weather Station reader program auriol-reader.
   
   This file was inspired by the rtl-wx project at GitHub
   https://github.com/magellannh/rtl-wx
   
   This file is part of the Weather Station reader program auriol-reader.
   
   The source code can be found at GitHub 
   https://github.com/yu55/auriol_pluviometer_reader
   
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

#include "config.h"

static const char LANG_CONF_OPEN_ERR[] = "ERROR: Can not open configuration file \"%s\" for reading!\n";

int confReadFile( char *inFname, ConfigSettings *conf ) {
	FILE *infd;
	char rdBuf[READ_BUFSIZE];
	
	/* Default Values */
	conf->serialDevice[0]   = 0;

	conf->mysqlServer[0]    = 0;
	conf->mysqlUser[0]      = 0;
	conf->mysqlPass[0]      = 0;
	conf->mysqlPort         = 3306;
	conf->mysqlDatabase[0]  = 0;
	
	conf->saveTemperatureTime = 3600;
	conf->saveHumidityTime    = 3600;
	
	if ( ( infd = fopen( inFname, "r" ) ) == NULL ) {
		fprintf( stderr, LANG_CONF_OPEN_ERR, inFname );
		return( 1 );
	}

	/* Read each line in the file and process the tags on that line. */
	while ( fgets( rdBuf, READ_BUFSIZE, infd ) != NULL ) {
		if ( ( rdBuf[0] != ';' ) && ( rdBuf[0] != '[' ) ) {
			if ( confStringVar( rdBuf, "serialDevice", conf->serialDevice ) ) {}
			
			else if ( confStringVar( rdBuf, "mysqlServer", conf->mysqlServer ) ) {}
			else if ( confStringVar( rdBuf, "mysqlUser", conf->mysqlUser ) ) {}
			else if ( confStringVar( rdBuf, "mysqlPass", conf->mysqlPass ) ) {}
			else if ( confIntVar( rdBuf, "mysqlPort", &conf->mysqlPort ) ) {}
			else if ( confStringVar( rdBuf, "mysqlDatabase", conf->mysqlDatabase ) ) {}
			else if ( confIntVar( rdBuf, "saveTemperatureTime", &conf->mysqlPort ) ) {}
			else if ( confIntVar( rdBuf, "saveHumidityTime", &conf->mysqlPort ) ) {}
		}
	}
	fclose( infd );
	
	/* Calculated Values */
	conf->mysql      = ( conf->mysqlServer[0] != 0 && conf->mysqlUser[0] != 0 && conf->mysqlPass[0] != 0 && conf->mysqlDatabase[0] != 0 );
	return( 0 );
}

/* Process a config file line with a single string var on it
 * Must be of the form 'StringVar=str' with no spaces or special characters before the '='.  After the = whitespace is treated
 * as part of the string. */
int confStringVar( char *buf, char *matchStr, char *destStr ) {
	int retVal = 0;
	int len    = strlen( matchStr );

	if ( strncmp( buf, matchStr, len ) == 0 ) {
		int i = len;
		int j = 0;

		if ( buf[i] != 0 ) /* skip '=' */
			i++;
		while ( ( ( buf[i] == ' ' ) || ( buf[i] =='\t' ) ) && ( i < READ_BUFSIZE ) )
			i++;
		while ( ( buf[i] != 0 ) && ( buf[i] != '\r' ) && ( buf[i] != '\n' ) 
			&& ( i < READ_BUFSIZE ) && ( j < ( MAX_CONFIG_NAME_SIZE - 1 ) ) )
			destStr[j++] = buf[i++];
		destStr[j] = 0;
		retVal     = 1;
	}
	return( retVal );
}

/* Process a config file line with a numeric var  on it
 * Must be of the form 'numericVar=n' with no spaces or special characters. */
int confIntVar( char *buf, char *matchStr, int *valp ) {
	int retVal = 0;
	int len    = strlen( matchStr );
	int varCount, temp = 0;

	if ( strncmp( buf, matchStr, len ) == 0 ) {
		varCount = sscanf( &buf[len+1], "%d", &temp );
		if ( varCount == 1 ) 
			*valp = temp;
		retVal = 1;
	}

	return( retVal );   
}

/* Process a config file line with a float var  on it
 * Must be of the form 'floatVar=n.n' with no spaces or special characters. */
int confFloatVar( char *buf, char *matchStr, float *valp ) {
	int retVal = 0;
	int len    = strlen( matchStr );
	int varCount;
	float temp = 0.0;

	if ( strncmp( buf, matchStr, len ) == 0 ) {
		varCount = sscanf( &buf[len+1], "%f", &temp );
		if ( varCount == 1 ) 
			*valp = temp;
		retVal = 1;
	}

	return( retVal );   
}
