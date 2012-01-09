include ../Makefile.inc

TOP=..

CFLAGS += -I$(TOP)/include

all: mali_info

clean:
	rm -f *.o
	rm -f mali_info

mali_info: mali_info.c
	$(CC) $(CFLAGS) -o $@ $^

install: $(ADB) mali_info
	$(ADB) push mali_info /system/bin

run:	$(ADB)
	$(ADB) shell mali_info

include ../Makefile.post
