#
# For Android, use:
# make CROSS_COMPILE=~/android/toolchain/bin/arm-linux-androideabi-
#

CFLAGS ?= -O0 -g -Wall

CROSS_COMPILE ?=
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld

triplet = $(shell $(CC) -dumpmachine)
ifeq ($(triplet), arm-linux-androideabi)
	CFLAGS += -DANDROID
	OS = android
else ifeq ($(triplet), arm-linux-gnueabi)
	OS = gnu
else ifeq ($(triplet), arm-linux-gnueabihf)
	OS = gnu
endif

ifndef OS
       $(error Unhandled architecture: $(triplet))
endif

ifeq ($(OS),android)
	ADB ?= adb
	DESTDIR ?=
	ifneq ($(DESTDIR),)
		INSTALL = install
	else
		INSTALL = $(ADB) push
	endif
	prefix ?= $(DESTDIR)/system/
else
	INSTALL = install
	ifneq ($(CROSS_COMPILE),)
		DESTDIR ?= /tmp/
	else
		DESTDIR ?=
	endif
	prefix ?= $(DESTDIR)/usr/
endif
