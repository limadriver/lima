include ../Makefile.inc

TOP=..

CFLAGS += -I$(TOP)/include

all: libMali_wrap.so

wrap.o: wrap.c

clean:
	rm -f *.o
	rm -f *.so

libMali_wrap.so: wrap.o
	$(CC) -shared -Wl -o $@ $^ -ldl

install: $(ADB) libMali_wrap.so
	$(ADB) push libMali_wrap.so /system/lib/

include ../Makefile.post
