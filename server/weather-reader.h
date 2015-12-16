#ifndef _WEATHER_READER_h_
#define _WEATHER_READER_h_

#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>
// #include <string.h>
#include <unistd.h>
#include "config.h"
#include "parser.h"
#include "sensor.h"
#include "server.h"
#include "uart.h"
#include "wire-sensor.h"

int pipeServer[2];
int pipeParser[2];

int main(int argc, char *argv[]);
void usage( void );
void signal_interrupt( int signum );
void signal_alarm( void );

#endif
