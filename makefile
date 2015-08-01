# File to compile the helopi example

server:
	$(MAKE) -C server all
	
arduino:
	$(MAKE) -C arduino all

install:
	$(MAKE) -C server install
	
uninstall:
	$(MAKE) -C server uninstall
	
.PHONY: server arduino install unbinstall
