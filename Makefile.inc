TOOLCHAIN ?= ~/android/toolchain/
SDK ?= ~/android/android-sdk-linux_x86/
ANDROID_TARGET ?= android-10

CC = $(TOOLCHAIN)/bin/arm-linux-androideabi-gcc
LD = $(TOOLCHAIN)/bin/arm-linux-androideabi-ld
ADB = $(SDK)/platform-tools/adb
SYSROOT = $(TOOLCHAIN)/sysroot/

SYSTEM = android
ARCH = arm

CFLAGS += -O0 -g -Wall

