PREFIX=/usr/
MYSQL_CFLAGS=$(shell mysql_config --cflags)
MYSQL_LIBS=$(shell mysql_config --libs)
GIT_VERSION := $(shell git describe --dirty --always --tags)
BUILD_DATE := $(shell date +%Y-%m-%d+%H:%M:%S )

all: weather-reader

weather-reader: weather-reader.c config.c parser.c sensor.c server.c uart.c wire-sensor.c i2c-sensor.c
	gcc -o ../bin/weather-reader \
		-D_DEBUG=0 \
		-DGITVERSION=\"$(GIT_VERSION)\" -DBUILDTIME=\"$(BUILD_DATE)\" \
		-pthread -lm \
		$(MYSQL_CFLAGS) \
		$(MYSQL_LIBS) \
		weather-reader.c config.c parser.c sensor.c server.c uart.c wire-sensor.c \
		i2c-sensor.c
	@echo
		
debug: weather-reader.c config.c parser.c sensor.c server.c uart.c wire-sensor.c i2c-sensor.c
	ulimit -c unlimited && gcc -o ../bin/weather-reader \
		-D_DEBUG=2 -g -ggdb -DVERSION=\"$(GIT_VERSION)\" \
		-DGITVERSION=\"$(GIT_VERSION)\" -DBUILDTIME=\"$(BUILD_DATE)\" \
		-pthread -lm \
		$(MYSQL_CFLAGS) \
		$(MYSQL_LIBS) \
		weather-reader.c config.c parser.c sensor.c server.c uart.c wire-sensor.c \
		i2c-sensor.c
	@echo
	
test-server: test/test_server.c
	gcc -o ../bin/test-server \
		-pthread -g \
		test/test_server.c
	@echo
	
test-client: test/test_client.c
	gcc -o ../bin/test-client \
		-pthread -g \
		test/test_client.c
	@echo
	
clean:
	- rm -f ../bin/weather-reader core

install: weather-reader
	install -m 0755    ../bin/weather-reader $(PREFIX)/bin
	install -m 0755 -d $(PREFIX)/share/weather-reader
	install -m 0755 -d $(PREFIX)/share/weather-reader/www
	install -m 0755    ../weather-reader/weather-reader $(PREFIX)/share/weather-reader
	install -m 0644    ../weather-reader/*.conf $(PREFIX)/share/weather-reader
	install -m 0644    ../weather-reader/www/* $(PREFIX)/share/weather-reader/www
	install -b -m 0644 ../weather-reader/weather-reader.conf /etc
	install -m 0755    ../weather-reader/weather-reader /etc/init.d
	update-rc.d weather-reader defaults 95 2> /dev/null

uninstall:
	- update-rc.d weather-reader remove 2> /dev/null
	- rm -fr $(PREFIX)/bin/weather-reader \
		$(PREFIX)/share/weather-reader \
		/etc/weather-reader.conf \
		/etc/init.d/weather-reader \
		/var/log/weather-reader.log

.PHONY: install uninstall clean
