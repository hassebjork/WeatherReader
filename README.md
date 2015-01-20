Weather-Reader
==============

This project uses an Arduino to read radio signals from wireless 
Weather Sensors. The Arduino is powered over USB and reduces payload 
on the Raspberry Pi. All sensors work over the 433MHz band. Data is 
sent over the USB port to a Raspberry Pi, reading it as a serial port. 
The raw data is displayed on stdout and stored in a MySQL database.

The MySQL database is configured in a configuration file. The file is 
read every hour, so changes can be implemented while running. 
The server can be set up to operate on localhost or a remote server.

Multiple Weather-Reader servers can be run simultaneously on a local 
network. They can manually be configured to receieve only certain sensors 
with the best reception. The servers sync their clocks against a MySQL 
database.


Directories:
============

arduino - Sketchbook for the arduino packet demodulizer & decoder. 
          Compile and upload with Arduino IDE. Set serial baudrate to 
          115200 view the raw data. The finished HEX file can also be 
          uploaded using avrdude:
          $ avrdude -C/etc/avrdude.conf -v -patmega328p -carduino \
                    -P/dev/ttyUSB0 -b57600 -D -Uflash:w:WeatherReader.cpp.hex:i

server  - Server program reading the arduino. It will be run on a Raspberry 
          Pi with Raspian linux. Configuration is done in the file
          /etc/weather-reader.conf
          
          Build the source files like this:
          $ make
          $ sudo make install
          $ weather-reader


Supported sensors:
==================

These sensors sholud work, but they have not all been tested. Sensors 
with the same protocol have been tested.

Oregon Scientific sensors (version 2.1) with temperature / humidity
  - THGR228N, THGN122N, THGN123N, THGR122NX, THGR238, THGR268
  - THGR328N (not time)
  - TRGR328N*
  - THGR810*
  - THGR918*, THGRN228NX*, THGN500*
Not working yet: THGN132N

Weather Stations with aneometer, pluviometer, temperature & humidity
  - Ventus WS155
  - Auriol H13726*
  - Hama EWS 1500*
  - Meteoscan W155/W160*

 Fine Offset protocol before 2012 (labled Viking, Prolouge)
  - Temperature & humidity*
  - Rain guage

 Mandolyn protocol (labled UPM or ESIC)
  - Temperature
  - Humidity
  - Wind*
  - Raind*

   * Not tested


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


ISSUES:
=======
- These cheap Arduino Nano boards, seem to have a ceramic ocillator. The clock speed 
  varies between different boards and reception of different protocols might not work 
  100% on every board.
  
- The Arduino sketch fails to decode some protocols after a few hours. Different protocols 
  drop out on different boards. A temporary workaround is implemented, to restart the 
  Arduino board every hour.
  
  