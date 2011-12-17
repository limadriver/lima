include ../Makefile.inc

all: premali

clean:
	rm -f *.o
	rm -f premali

premali: premali.c dumped_stream.c
	$(CC) $(CFLAGS) -o $@ premali.c -lEGL_mali -lGLESv2_mali

install: $(SDK)/platform-tools/adb premali
	$(SDK)/platform-tools/adb push premali /system/bin

wrap: $(SDK)/platform-tools/adb
	$(SDK)/platform-tools/adb shell "LD_PRELOAD=libMali_wrap.so premali"

run:	$(SDK)/platform-tools/adb
	$(SDK)/platform-tools/adb shell premali

include ../Makefile.post
