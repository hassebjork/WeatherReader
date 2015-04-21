Weather-Reader
==============

This project uses a Raspberry Pi to receive data from ordinary 
weather sensors, such as temperature, humidity, wind and rain. The 
data is stored in a MySQL-database, on a local or remote server.

An Arduino (Nano) connected to the Raspberry Pi via USB, is used to 
recieve radio signals from the sensors. The Arduino is powerd through 
the USB socket and uses the built in USB-to-serial connection to 
communicate with the Raspberry Pi. It will reduce the payload of the 
Raspberry Pi significantly.

Multiple Arduinos can be connected to one Raspberry Pi, or muliple 
Raspberry Pis with an Arduino each can be used to increase the 
reception range. Each Raspberry can be configured to send data to the 
same database or one Raspberry Pi can be run in server mode, 
receiving data from the other Raspberry Pis. This will reduce the 
amount of data stored in the database. The server listens to port 
9876 as default and data is sent over UDP.

To further reduce data storage, data is only stored when it has 
changed. If it remains constant, data will be stored every hour 
(can be configured), to make it easier to graph on an hourly basis.

Supported sensors are Oregon Scientific (protocol 2.1 tested sensors 
THGR122N, THGN132N & THGR328N), mandolyn protocol (tested UMP and 
ESIC) and Weather Station Ventus WS155 (same as Auriol H13726, Hama 
EWS 1500 and Meteoscan W155/W160). It has also been tested with 
rain guage using the FineOffset protocol (labeld Prolouge or Viking). 
Temperature and humidity sensors using FineOffset protocol should 
work. Same goes for mandolyn protocol weather stations.

Directories:
============

arduino - Sketches for Arduinos, source code and compiled hex files
        * WeatherReader is used for reading radio data
        * WireSensor is (in progress) used for reading manually wired 
        sensors, like DS18B20, DHT22 or digital signals
        
        Uploading pre compiled HEX files to an Arduino throug the
        USB-serial cable is done with avrdude:
          $ avrdude -C/etc/avrdude.conf -v -patmega328p -carduino \
              -P/dev/ttyUSB0 -b57600 -D -Uflash:w:WeatherReader.hex:i

server - Source code for the server program communicating with the 
        Arduinos. It will be run on a Raspberry Pi with Raspian linux. 
        Configuration is done in the file /etc/weather-reader.conf
        
        Build the source files like this:
          $ sudo apt-get install libmysqlclient-dev
          $ make
          
        Installation of files:
          $ sudo make install
        
        Setup the database and mode of operation in the config file
          $ sudo nano /etc/weather-reader.conf
          
        Run the program manually
          $ weather-reader

weather-reader - Various files used by the server, for installation, 
        configuration and setting up Apache2 or Lighthttpd servers.
        There is also a basic PHP file for displaying weather 
        readings for the last 24 hours.
          
HARDWARE:
=========

My heardware setup is a LAMP server, Raspberry Pi, Arduino Nano and a receiver.
The LAMP server (Linux, Apache2, MySQL, PHP) is used for data storage and 
(yet to come) displaying the data on a web page.

The Raspberry Pi decodes data and sends it to the database. In theory, it could 
hold the LAMP server too, but many write cycles to the SD card will wear it out.

Connected to a USB port on the Raspberry Pi, is an Arduino Nano (US$ 3). It draws 
power from the USB and sends serial data back to the Raspberry Pi. It decodes the 
radio signal and tries to verify the checksum, before transmitting the data for 
decoding.

The Arduino has built in interrupts and a timer, which makes it very efficient at 
decoding the signal lengths, while still running other code. The Raspberry Pi running 
a polling algorithm will use 98% of the CPU time (using Wiring Pi) and any additional 
load on the CPU will disturb the timing calculations.

As a receiver I use a 433 MHz receiver shield module called RXB6. It is sensitive 
and cheap (US$ 3.5). Any similar receiver should do. It connects to the Arduino Nanos 
pins +5V, GND and data to analogue pin 1 (A1). The rest of the pins are unused.


OPERATION OF SOFTWARE:
======================

The configuration file is first read starting some threads: 
   UART   - Receives raw signals from Arduino and sends them to a) 
            parser or b) client thread. A UART-thread is started for
            every ttyUSBx device seen in the directory /dev.
   PARSER - Decodes messages, filters and store them in database
   CLIENT - Transmits signals to another Raspberry Pi server
   SERVER - Recieves raw signals from multiple clients for parsing 
            and filtering

a. Default: UART sends to PARSER wich stores in database
```
   +--------+      +--------+               +-------+
   |  UART  | -->  | PARSER | --[lo/net]--> | MySQL |
   +--------+      +--------+               +-------+
```
b. Client-mode: UART sends to CLIENT wich over network sends to a server 
```
   +--------+      +--------+                +--------+
   |  UART  | -->  | CLIENT | --[network]--> | SERVER |
   +--------+      +--------+                +--------+
```
c. Server-mode: UART and network SERVER sends to PARSER wich stores in database 
```
   +--------+      +--------+     +--------+
   |  UART  | -->  | PARSER | <-- | SERVER | <--[network]--
   +--------+      +--------+     +--------+
                       |  local or
                       V  network
                   +-------+
                   | MySQL |
                   +-------+
```
The reason for using a separate thread for the parser, is because MySQL is 
not thread-safe causing problems when using server + uart simultaneously.

ISSUES:
=======
- These cheap Arduino Nano boards, seem to have a ceramic ocillator. The clock speed 
  varies between different boards and reception of different protocols might not work 
  100% on every board.

- The Arduino sketch fails to decode some protocols after a few hours. Different protocols 
  drop out on different boards. A temporary workaround is implemented, to restart the 
  Arduino board every hour.
