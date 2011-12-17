include Makefile.inc

all:
	cd wrap && $(MAKE) all
	cd test && $(MAKE) all
	cd premali && $(MAKE) all

clean:
	cd wrap && $(MAKE) clean
	cd test && $(MAKE) clean
	cd premali && $(MAKE) clean

install:
	make remount
	cd wrap && $(MAKE) install
	cd test && $(MAKE) install
	cd premali && $(MAKE) install

include Makefile.post
