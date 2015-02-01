#ifndef _WEATHER_READER_h_
#define _WEATHER_READER_h_

#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <termios.h>
#include <unistd.h>
#include "config.h"
#include "sensor.h"
#include "server.h"

int main(int argc, char *argv[]);
void signal_interrupt( int signum );
void signal_alarm( void );
void reset_arduino( void );

void *uart_receive( void *ptr );
unsigned char hex2char( unsigned char c );
unsigned char reverse_8bits( unsigned char n );
void parse_input( char *s );

void osv2_parse( char *s );
unsigned char osv2_humidity( char *s );
float osv2_temperature( char *s );
void vent_parse( char *s );
void mandolyn_parse( char *s );
void fineoffset_parse( char *s );

#endif
