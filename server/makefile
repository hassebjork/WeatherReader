# File to compile the helopi example
PREFIX=/usr/local
MYSQL_CFLAGS=$(shell mysql_config --cflags)
MYSQL_LIBS=$(shell mysql_config --libs)


all: weatherreader

weatherreader: weatherreader.c config.c sensor.c
	gcc -o weather-reader \
		-lpthread \
		$(MYSQL_CFLAGS) \
		$(MYSQL_LIBS) \
		weatherreader.c config.c sensor.c
		
debug: weatherreader.c config.c sensor.c
	ulimit -c unlimited && gcc -o weather-reader \
		-D_DEBUG=2 \
		-lpthread -ggdb \
		$(MYSQL_CFLAGS) \
		$(MYSQL_LIBS) \
		weatherreader.c config.c sensor.c

