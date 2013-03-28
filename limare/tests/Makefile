DIRS =	common \
	triangle_smoothed \
	strip_smoothed \
	fan_smoothed \
	quad_flat \
	triangle_quad \
	cube_smoothed \
	cube_smoothed_spinning \
	quad_textured \
	cube_textured \
	cube_textured_spinning \
	cube_companion \
	cube_companion_spinning \
	cube_textured_indexed \
	quad_direct \
	multiple \
	cube_companion_bo \
	cube_companion_bo_indexed \
	gles1_clear \

.PHONY: all clean install $(DIRS)

all: MAKE = make all
all: $(DIRS)

$(DIRS):
	$(MAKE) -C $@

install: MAKE = make install
install: $(DIRS)

clean: MAKE = make clean
clean: $(DIRS)
