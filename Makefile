UNAME_SYSTEM := $(shell uname -s)

ifeq ($(UNAME_SYSTEM),Linux)
    RAYLIB_PATH ?= raylib/raylib-5.5-raspberry
    RAYLIB_LIBS ?= -lm -lGLESv2 -lEGL -ldrm -lgbm
else ifeq ($(UNAME_SYSTEM),Darwin)
    RAYLIB_PATH ?= raylib/raylib-5.5-macos
    RAYLIB_LIBS ?= -lm -framework AppKit -framework IOKit
endif

RAYLIB_PATH ?= raylib/raylib-5.5-raspberry
RAYLIB_LIBS ?= -lm -lGLESv2 -lEGL -ldrm -lgbm

CFLAGS ?= -I$(RAYLIB_PATH)/include $(RAYLIB_LIBS)
LDLIBS ?= $(RAYLIB_PATH)/libraylib.a

ALL_PROGRAMS := random_pixels snake_animation text

.PHONY: all
all: $(ALL_PROGRAMS)

random_pixels: random_pixels.c
snake_animation: snake_animation.c
text: text.c

clean:
	rm -f $(ALL_PROGRAMS)
