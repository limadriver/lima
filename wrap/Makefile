include ../Makefile.inc

TOP=..

CFLAGS += -I$(TOP)/include

all: libMali_wrap.so

bmp.o: bmp.c bmp.h

wrap.o: wrap.c bmp.h

clean:
	rm -f *.o
	rm -f *.so

libMali_wrap.so: bmp.o wrap.o
	$(CC) -shared -Wl -o $@ $^ -ldl

install: libMali_wrap.so
	$(INSTALL) libMali_wrap.so $(prefix)/lib/

include ../Makefile.post
