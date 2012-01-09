TOP = ../../../

include $(TOP)Makefile.inc

CFLAGS += -I$(TOP)include -I../../lib/

.PHONY : all clean egl premali install run wrap egl_install egl_run egl_wrap

all: premali egl

clean:
	rm -f *.o
	rm -f premali_triangle_smoothed_inverted
	rm -f egl_triangle_smoothed_inverted

# Premali version of render.

premali: premali_triangle_smoothed_inverted

premali_triangle_smoothed_inverted: premali.c
	$(CC) $(CFLAGS) -o $@ $^ -lpremali

install: $(ADB) premali_triangle_smoothed_inverted
	$(ADB) push premali_triangle_smoothed_inverted /system/bin/premali/premali/triangle_smoothed_inverted

run: $(ADB)
	$(ADB) shell "rm /sdcard/premali.bmp"
	$(ADB) shell /system/bin/premali/premali/triangle_smoothed_inverted
	$(ADB) pull /sdcard/premali.bmp

wrap: $(ADB)
	$(ADB) shell "rm /sdcard/premali.bmp"
	$(ADB) shell "rm /sdcard/premali.wrap.log"
	$(ADB) shell "LD_PRELOAD=libMali_wrap.so REMALI_WRAP_LOG=/sdcard/premali.wrap.log /system/bin/premali/premali/triangle_smoothed_inverted"
	$(ADB) pull /sdcard/premali.bmp
	$(ADB) pull /sdcard/premali.wrap.log

# EGL -- original code.

egl: egl_triangle_smoothed_inverted

egl_triangle_smoothed_inverted: egl.c
	$(CC) $(CFLAGS) -o $@ $^ -lEGL_mali -lGLESv2_mali

egl_install: $(ADB) egl_triangle_smoothed_inverted
	$(ADB) push egl_triangle_smoothed_inverted /system/bin/premali/egl/triangle_smoothed_inverted

egl_wrap: $(ADB)
	$(ADB) shell "rm /sdcard/remali_test.wrap.log"
	$(ADB) shell "LD_PRELOAD=libMali_wrap.so REMALI_WRAP_LOG=/sdcard/remali_test.wrap.log /system/bin/premali/egl/triangle_smoothed_inverted"
	$(ADB) pull /sdcard/remali_test.wrap.log

egl_run: $(ADB)
	$(ADB) shell /system/bin/premali/egl/triangle_smoothed_inverted

include $(TOP)Makefile.post
