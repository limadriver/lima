TOOLCHAIN ?= ~/android/toolchain/
SDK ?= ~/android/android-sdk-linux_x86/

CC = $(TOOLCHAIN)/bin/arm-linux-androideabi-gcc
LD = $(TOOLCHAIN)/bin/arm-linux-androideabi-ld
SYSROOT = $(TOOLCHAIN)/sysroot/

CFLAGS += -O0 -g -Wall

SYSTEM = android
ARCH = arm

