RAYLIB_PATH ?= raylib/raylib-5.5-raspberry
CFLAGS ?= -I$(RAYLIB_PATH)/include
LDLIBS ?= $(RAYLIB_PATH)/libraylib.a -lm -lGLESv2 -lEGL -ldrm -lgbm

ALL_PROGRAMS := random_pixels snake_animation text

.PHONY: all
all: $(ALL_PROGRAMS)

random_pixels: random_pixels.c
snake_animation: snake_animation.c
text: text.c

clean:
	rm -f $(ALL_PROGRAMS)
