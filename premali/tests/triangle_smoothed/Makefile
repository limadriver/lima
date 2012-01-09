TOP = ../../../

include $(TOP)Makefile.inc

CFLAGS += -I$(TOP)include -I../../lib/

.PHONY : all clean egl premali install run wrap egl_install egl_run egl_wrap

all: premali egl

clean:
	rm -f *.o
	rm -f premali_triangle_smoothed
	rm -f egl_triangle_smoothed

# Premali version of render.

premali: premali_triangle_smoothed

premali_triangle_smoothed: premali.c
	$(CC) $(CFLAGS) -o $@ $^ -lpremali

install: $(ADB) premali_triangle_smoothed
	$(ADB) push premali_triangle_smoothed /system/bin/premali/premali/triangle_smoothed

run: $(ADB)
	$(ADB) shell "rm /sdcard/premali.bmp"
	$(ADB) shell /system/bin/premali/premali/triangle_smoothed
	$(ADB) pull /sdcard/premali.bmp

wrap: $(ADB)
	$(ADB) shell "rm /sdcard/premali.bmp"
	$(ADB) shell "rm /sdcard/premali.wrap.log"
	$(ADB) shell "LD_PRELOAD=libMali_wrap.so REMALI_WRAP_LOG=/sdcard/premali.wrap.log /system/bin/premali/premali/triangle_smoothed"
	$(ADB) pull /sdcard/premali.bmp
	$(ADB) pull /sdcard/premali.wrap.log

# EGL -- original code.

egl: egl_triangle_smoothed

egl_triangle_smoothed: egl.c
	$(CC) $(CFLAGS) -o $@ $^ -lEGL_mali -lGLESv2_mali

egl_install: $(ADB) egl_triangle_smoothed
	$(ADB) push egl_triangle_smoothed /system/bin/premali/egl/triangle_smoothed

egl_wrap: $(ADB)
	$(ADB) shell "rm /sdcard/remali_test.wrap.log"
	$(ADB) shell "LD_PRELOAD=libMali_wrap.so REMALI_WRAP_LOG=/sdcard/remali_test.wrap.log /system/bin/premali/egl/triangle_smoothed"
	$(ADB) pull /sdcard/remali_test.wrap.log

egl_run: $(ADB)
	$(ADB) shell /system/bin/premali/egl/triangle_smoothed

include $(TOP)Makefile.post
