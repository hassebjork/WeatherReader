Weather-Reader
==============

This project uses an Arduino to read radio signals from wireless 
Weather Sensors. The Arduino is powered over USB and reduces payload 
on the Raspberry Pi. All sensors work over the 433MHz band. Data is 
sent over the USB port to a Raspberry Pi, reading it as a serial port. 
The raw data is displayed on stdout and stored in a MySQL database.

The MySQL database is configured in a configuration file. It can be 
set up to operate locally or on a remote server.

Multiple Weather-Reader servers can be run simultaneously on a local 
network. They can be configured to receieve only certain sensors with 
the best reception.

Directories:
============

arduino - Sketchbook for the arduino packet demodulizer & decoder. 
          Compile and upload with Arduino IDE. Set serial baudrate to 
          115200 view the raw data.

server  - Program reading the arduino and storing data to database. 
          Build the source files like this:
          $ make

Supported sensors:
==================

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
  - Wind
  - Raind

   * Not tested
