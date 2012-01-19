TOP=../..

include $(TOP)/Makefile.inc

CFLAGS += -I$(TOP)/include

all: mali_compile

clean:
	rm -f *.o
	rm -f mali_compile

mali_compile: compile.c
	$(CC) $(CFLAGS) -o $@ $^ -lMali

install: $(ADB) mali_compile
	$(ADB) push mali_compile /system/bin

run:	$(ADB)
	$(ADB) shell mali_compile -v /sdcard/shader.txt

include $(TOP)/Makefile.post
