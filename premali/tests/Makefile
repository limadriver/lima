DIRS = triangle_smoothed \
	triangle_smoothed_inverted

.PHONY: all clean install $(DIRS)

all: MAKE = make all
all: $(DIRS)

$(DIRS):
	$(MAKE) -C $@

install: MAKE = make install
install: $(DIRS)

clean: MAKE = make clean
clean: $(DIRS)
