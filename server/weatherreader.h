#ifndef _WEATHER_READER_h_
#define _WEATHER_READER_h_

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <signal.h>
#include "config.h"
#include "sensor.h"

void signal_callback_handler( int signum );

unsigned char hex2char( unsigned char c );
unsigned char reverse_8bits( unsigned char n );
unsigned char osv2_humidity( char *s );
float osv2_temperature( char *s );
void osv2_parse( char *s );
void vent_parse( char *s );
void parse_input( char *s );

void *uart_receive( void *ptr );

int main(int argc, char *argv[]);

#endif
