# File to compile the helopi example
PREFIX=/usr/
MYSQL_CFLAGS=$(shell mysql_config --cflags)
MYSQL_LIBS=$(shell mysql_config --libs)


all: weather-reader

weather-reader: weather-reader.c config.c parser.c sensor.c server.c uart.c
	gcc -o ../bin/weather-reader \
		-D_DEBUG=0 -g \
		-pthread \
		$(MYSQL_CFLAGS) \
		$(MYSQL_LIBS) \
		weather-reader.c config.c parser.c sensor.c server.c uart.c
	@echo
		
debug: weather-reader.c config.c parser.c sensor.c server.c uart.c
	ulimit -c unlimited && gcc -o ../bin/weather-reader \
		-D_DEBUG=2 -g \
		-pthread -ggdb \
		$(MYSQL_CFLAGS) \
		$(MYSQL_LIBS) \
		weather-reader.c config.c parser.c sensor.c server.c uart.c
	@echo

clean:
	- rm -f weather-reader core

install: weather-reader
	install -m 0755 ../bin/weather-reader $(PREFIX)/bin
	install -m 0755 -d ../weather-reader $(PREFIX)/share/weather-reader
	install -m 0755 weather-reader /etc/init.d
	install -b -m 0644 ../weather-reader/weather-reader.conf /etc
	update-rc.d weather-reader defaults 95

uninstall:
	-rm -f $(PREFIX)/bin/weather-reader
	-rm -f /etc/weather-reader.conf

.PHONY: install uninstall clean
