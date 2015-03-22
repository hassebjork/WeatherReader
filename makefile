# File to compile the helopi example

server:
	$(MAKE) -C server all
	
arduino:
	$(MAKE) -C arduino all

.PHONY: server arduino
