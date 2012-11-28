TOP=../..

include $(TOP)/Makefile.inc

CFLAGS += -I$(TOP)/include

OBJS = info.o

all: mali_info

clean:
	rm -f *.P
	rm -f *.o
	rm -f mali_info

mali_info: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

install: mali_info
	$(INSTALL) $^ $(prefix)/bin

run:
ifeq ($(OS),android)
	$(ADB) shell mali_info
endif

include $(TOP)/Makefile.post
