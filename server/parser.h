/*************************************************************************
   parser.h

   This module decodes the radio codes from the Arduino.
   
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

#ifndef _PARSER_H_
#define _PARSER_H_

#include <mysql.h>
#include <stdio.h>
#include "config.h"
#include "sensor.h"
#include "weather-reader.h"

void *parse_thread();
unsigned char hex2char( unsigned char c );
unsigned char reverse_8bits( unsigned char n );
void parse_input( char *s );

void osv2_parse( char *s );
unsigned char osv2_humidity( char *s );
float osv2_temperature( char *s );
void vent_parse( char *s );
void mandolyn_parse( char *s );
void fineoffset_parse( char *s );
void wired_parse( char *s );
void json_parse( char *s );

void json_parseString( char *p, char *s );
void json_parseInt( char *p, int *value );
void json_parseFloat( char *p, float *value );
void json_parseWhitespace( char *p );

#endif