TOP = ../../

include $(TOP)Makefile.inc

CFLAGS += -I$(TOP)include

all: liblimare.so

clean:
	rm -f *.o
	rm -f *.so

bmp.o: bmp.c bmp.h

fb.o: fb.c fb.h

hfloat.o: hfloat.c hfloat.h

symbols.o: symbols.c symbols.h

plb.o: plb.c plb.h premali.h

jobs.o: jobs.c jobs.h premali.h

dump.o: dump.c dump.h premali.h

gp.o: gp.c gp.h premali.h plb.h symbols.h

pp.o: pp.c pp.h premali.h plb.h

program.o: program.c program.h gp.h

premali.o: premali.c premali.h

liblimare.so: bmp.o fb.o plb.o hfloat.o symbols.o jobs.o dump.o gp.o pp.o program.o premali.o
	$(CC) -shared -Wall -o $@ $^ -lMali

install: $(ADB) liblimare.so
	cp liblimare.so $(SYSROOT)usr/lib/
	$(ADB) push liblimare.so /system/lib/

include $(TOP)Makefile.post
