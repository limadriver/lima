include ../Makefile.inc

all: libMali_wrap.so




clean:
	rm -f *.o
	rm -f *.so

libMali_wrap.so:
	$(CC) -shared -Wl -o $@ $^ -ldl

install: $(SDK)/platform-tools/adb libMali_wrap.so
	$(SDK)/platform-tools/adb push libMali_wrap.so /system/lib/

include ../Makefile.post
