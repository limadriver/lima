include ../Makefile.inc

all: premali

clean:
	rm -f *.o
	rm -f premali

bmp.o: bmp.c bmp.h

premali.o: premali.c dumped_stream.c bmp.h
	$(CC) $(CFLAGS) -c -o $@ premali.c

premali: bmp.o premali.o
	$(CC) $(CFLAGS) -o $@ $^

install: $(SDK)/platform-tools/adb premali
	$(SDK)/platform-tools/adb push premali /system/bin

wrap: $(SDK)/platform-tools/adb
	$(SDK)/platform-tools/adb shell "rm /sdcard/premali.bmp"
	$(SDK)/platform-tools/adb shell "LD_PRELOAD=libMali_wrap.so premali" > premali.log
	$(SDK)/platform-tools/adb pull /sdcard/premali.bmp
	

run:	$(SDK)/platform-tools/adb
	$(SDK)/platform-tools/adb shell "rm /sdcard/premali.bmp"
	$(SDK)/platform-tools/adb shell premali
	$(SDK)/platform-tools/adb pull /sdcard/premali.bmp

include ../Makefile.post
