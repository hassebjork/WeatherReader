WeatherReader
=============

An Arduino reading weather sensors, sending data through USB/serial to a Raspberry Pi, storing the data.


This project uses an Arduino to read radio signals from wireless Weather Sensors. The Arduino is 
powered over USB and reduces payload on the Raspberry Pi.
All sensors work over the 433MHz band. 
Data is sent over the USB port to a Raspberry Pi, reading it as a serial port. The data is currently 
displayed on stdout.

In the future the data will be stored in a MySQL database, locally or on a remote server. Optionally 
a SQLite3 database will store data locally.

Currently supported sensors:

Oregon Scientific sensors with temperature / humidity
- THGR228N, THGN122N, THGN123N, THGR122NX, THGR238, THGR268
- THGR328N (not time)
- TRGR328N*
- THGR810*
- THGR918*, THGRN228NX*, THGN500*
* Not tested

Weather Stations with aneometer, pluviometer, temperature & humidity
- Ventus WS155
- Auriol H13726*
- Hama EWS 1500*
- Meteoscan W155/W160*
* Not tested
