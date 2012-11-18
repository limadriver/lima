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

plb.o: plb.c plb.h limare.h

jobs.o: jobs.c jobs.h limare.h

dump.o: dump.c dump.h limare.h

gp.o: gp.c gp.h limare.h plb.h symbols.h

pp.o: pp.c pp.h limare.h plb.h

program.o: program.c program.h gp.h

limare.o: limare.c limare.h

liblimare.so: bmp.o fb.o plb.o hfloat.o symbols.o jobs.o dump.o gp.o pp.o program.o limare.o
	$(CC) -shared -Wall -o $@ $^ -lMali

install: liblimare.so
	$(INSTALL) liblimare.so $(prefix)/lib/

include $(TOP)Makefile.post
