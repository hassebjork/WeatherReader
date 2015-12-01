/*************************************************************************
   uart.h

   This module communicates with the Arduino over serial USB.
   
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

#ifndef _UART_H_
#define _UART_H_

#include <dirent.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include "config.h"
#include "parser.h"
#include "server.h"
#include "weather-reader.h"

// Time to withhold a resend of identical data
#define QUEUE_TIME     15

#define UNDEFINED      0
#define WEATHER_READER 1
#define WIRE_SENSOR    2
#define SENSOR_READER  3

static const char * SERIAL_TYPES[] =  { "Undefined", "Weather-reader", "Wire-sensor", "Sensor-reader" };

struct UARTQueueNode {
	time_t                time; 	// Node time
	char                 *s;		// Received string
	struct UARTQueueNode *link;		// Next node
};

typedef struct {
	int   tty;
	int   no;
	int   active;
	int   type;
	char *name;
	struct UARTQueueNode *head;		// Fisrst stored value
	struct UARTQueueNode *tail;		// Last stored value
} SerialDevice;

void *uart_receive( void *ptr );
char uart_checkQueue( SerialDevice *sDev, char *s );
void uart_handleData( SerialDevice *sDev, char *s, int rcount );
void uart_init( SerialDevice *sDev );
void reset_arduino( SerialDevice *sDev );
char **uart_get_serial( void );
#endif