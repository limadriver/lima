include ../Makefile.inc

all: mali_info

clean:
	rm -f *.o
	rm -f mali_info

mali_info: mali_info.c
	$(CC) $(CFLAGS) -o $@ $^

install: $(SDK)/platform-tools/adb mali_info
	$(SDK)/platform-tools/adb push mali_info /system/bin

run:	$(SDK)/platform-tools/adb
	$(SDK)/platform-tools/adb shell mali_info

include ../Makefile.post
