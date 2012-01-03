include ../Makefile.inc

all: premali

clean:
	rm -f *.o
	rm -f premali

bmp.o: bmp.c bmp.h

fb.o: fb.c fb.h

symbols.o: symbols.c symbols.h

plb.o: plb.c plb.h premali.h

jobs.o: jobs.c jobs.h premali.h

dump.o: dump.c dump.h premali.h

gp.o: gp.c gp.h premali.h plb.h symbols.h

pp.o: pp.c pp.h premali.h plb.h

premali.o: premali.c bmp.h fb.h symbols.h plb.h jobs.h gp.h pp.h premali.h
	$(CC) $(CFLAGS) -c -o $@ premali.c

premali: bmp.o fb.o plb.o symbols.o jobs.o dump.o gp.o pp.o premali.o
	$(CC) $(CFLAGS) -o $@ $^

install: $(SDK)/platform-tools/adb premali
	$(SDK)/platform-tools/adb push premali /system/bin

wrap: $(SDK)/platform-tools/adb
	$(SDK)/platform-tools/adb shell "rm /sdcard/premali.bmp"
	$(SDK)/platform-tools/adb shell "rm /sdcard/premali.wrap.log"
	$(SDK)/platform-tools/adb shell "LD_PRELOAD=libMali_wrap.so REMALI_WRAP_LOG=/sdcard/premali.wrap.log premali"
	$(SDK)/platform-tools/adb pull /sdcard/premali.bmp
	$(SDK)/platform-tools/adb pull /sdcard/premali.wrap.log

run:	$(SDK)/platform-tools/adb
	$(SDK)/platform-tools/adb shell "rm /sdcard/premali.bmp"
	$(SDK)/platform-tools/adb shell premali
	$(SDK)/platform-tools/adb pull /sdcard/premali.bmp

include ../Makefile.post
