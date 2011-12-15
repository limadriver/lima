include Makefile.inc

all:
	cd wrap && $(MAKE) all
	cd test && $(MAKE) all

clean:
	cd wrap && $(MAKE) clean
	cd test && $(MAKE) clean

install:
	make remount
	cd wrap && $(MAKE) install
	cd test && $(MAKE) install

include Makefile.post
