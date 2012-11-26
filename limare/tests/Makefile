DIRS =	common \
	triangle_smoothed \
	strip_smoothed \
	fan_smoothed \
	quad_flat \
	triangle_quad \
	cube_smoothed

.PHONY: all clean install $(DIRS)

all: MAKE = make all
all: $(DIRS)

$(DIRS):
	$(MAKE) -C $@

install: MAKE = make install
install: $(DIRS)

clean: MAKE = make clean
clean: $(DIRS)
