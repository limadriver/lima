TOP=../..

include $(TOP)/Makefile.inc

CFLAGS += -I$(TOP)/include

all: mali_info

clean:
	rm -f *.o
	rm -f mali_info

mali_info: info.c
	$(CC) $(CFLAGS) -o $@ $^

install: mali_info
	$(INSTALL) mali_info $(prefix)/bin

run:
ifeq ($(OS),android)
	$(ADB) shell mali_info
endif

include $(TOP)/Makefile.post
