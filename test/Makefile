include ../Makefile.inc

all: remali_test

clean:
	rm -f *.o
	rm -f remali_test

remali_test: test.c
	$(CC) $(CFLAGS) -o $@ $^ -lEGL_mali -lGLESv2_mali

install: $(SDK)/platform-tools/adb remali_test
	$(SDK)/platform-tools/adb push remali_test /system/bin

wrap: $(SDK)/platform-tools/adb
	$(SDK)/platform-tools/adb shell "LD_PRELOAD=libMali_wrap.so remali_test"

run:	$(SDK)/platform-tools/adb
	$(SDK)/platform-tools/adb shell remali_test

include ../Makefile.post
