UNAME_SYSTEM := $(shell uname -s)

ifeq ($(UNAME_SYSTEM),Linux)
    RAYLIB_PATH ?= ../raylib/raylib-5.5-raspberry
    RAYLIB_LIBS ?= -lm -lGLESv2 -lEGL -ldrm -lgbm
else ifeq ($(UNAME_SYSTEM),Darwin)
    RAYLIB_PATH ?= ../raylib/raylib-5.5-macos
    RAYLIB_LIBS ?= -lm -framework AppKit -framework IOKit
endif

CFLAGS ?= -I$(RAYLIB_PATH)/include $(RAYLIB_LIBS) -g
CXXFLAGS ?= $(CFLAGS) -std=c++11
LDLIBS ?= $(RAYLIB_PATH)/libraylib.a

ALL_PROGRAMS := pixelwall

.PHONY: all
all: $(ALL_PROGRAMS)

pixelwall: pixelwall.c design_snake_animation.c design_random_pixels.c design_text.c design_pong.c design_life.c

clean:
	rm -f $(ALL_PROGRAMS)
	rm -fr *.dSYM
