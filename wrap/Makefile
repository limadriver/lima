include ../Makefile.inc

TOP=..

CFLAGS += -I$(TOP)/include -fPIC

OBJS = bmp.o wrap.o

all: libMali_wrap.so

clean:
	rm -f *.P
	rm -f *.o
	rm -f *.so

libMali_wrap.so: $(OBJS)
	$(CC) -shared -Wl -o $@ $^ -ldl

install: libMali_wrap.so
	$(INSTALL) libMali_wrap.so $(prefix)/lib/

include ../Makefile.post
