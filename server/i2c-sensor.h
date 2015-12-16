/*************************************************************************
   i2c-sensor.h

   This module decodes various sensors connected to the I2C bus.
   Supported sensors: BMP085, BMP180
   
   This module requires the kernel module i2c-dev to be loaded
   
   Configuration is done in the file weather-reader.conf
   
   This file is part of the Weather Station reader program WeatherReader.
   This module has been modified from these sources:
      https://github.com/maasdoel/bmp180
      http://bunniestudios.com/blog/images/infocast_i2c.c
   
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

#ifndef _I2C_SENSORS_H_
#define _I2C_SENSORS_H_

#define sleepms(ms)  usleep((ms)*1000)
#define I2CBus             "/dev/i2c-1"      //New Pi's
//#define I2CBus             "/dev/i2c-0"    //Old,  but not stale Pi's

#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <sys/ioctl.h>
#include <math.h>

static int i2c_read        (int file, uint8_t addr, uint8_t reg, uint8_t bytes, uint8_t *val );
static int i2c_write       (int file, uint8_t addr, uint8_t reg, uint8_t value );
int        i2c_bmp085      (int fd, double *temperature, double *pressure );
int        i2c_bmp085_wait (int fd);
// double     bmp_altitude    (double p);
// double     bmp_qnh         (double p,double StationAlt);
// double     ppl_DensityAlt  (double PAlt,double Temp);

#endif