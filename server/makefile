# File to compile the helopi example
PREFIX=/usr/
MYSQL_CFLAGS=$(shell mysql_config --cflags)
MYSQL_LIBS=$(shell mysql_config --libs)


all: weatherreader

weatherreader: weatherreader.c config.c sensor.c server.c
	gcc -o weather-reader \
		-lpthread \
		$(MYSQL_CFLAGS) \
		$(MYSQL_LIBS) \
		weatherreader.c config.c sensor.c server.c
	@echo
		
debug: weatherreader.c config.c sensor.c server.c test/test_server.c
	ulimit -c unlimited && gcc -o weather-reader \
		-D_DEBUG=2 \
		-lpthread -ggdb \
		$(MYSQL_CFLAGS) \
		$(MYSQL_LIBS) \
		weatherreader.c config.c sensor.c server.c
	gcc -o test-server -lpthread test/test_server.c
	@echo

clean:
	- rm -f weather-reader core

install: weather-reader
	install -m 0755 weather-reader $(PREFIX)/bin
	install -m 0755 weather-reader.sh /etc/init.d
	install -b -m 0644 weather-reader.conf /etc

uninstall:
	-rm -f $(PREFIX)/bin/weather-reader
	-rm -f /etc/weather-reader.conf

.PHONY: install uninstall clean
