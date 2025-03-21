UNAME_SYSTEM := $(shell uname -s)

ifeq ($(UNAME_SYSTEM),Linux)
    RAYLIB_PATH ?= ../raylib/raylib-5.5-raspberry
    RAYLIB_LIBS ?= -lm -lGLESv2 -lEGL -ldrm -lgbm
else ifeq ($(UNAME_SYSTEM),Darwin)
    RAYLIB_PATH ?= ../raylib/raylib-5.5-macos
    RAYLIB_LIBS ?= -lm -framework AppKit -framework IOKit
endif

CFLAGS ?= -I$(RAYLIB_PATH)/include -g
CXXFLAGS ?= $(CFLAGS) -std=c++11
LDLIBS ?= $(RAYLIB_PATH)/libraylib.a $(RAYLIB_LIBS)


PROGRAM = pixelwall
OBJECTS = \
	pixelwall.o              \
	font.o                   \
	design_snake_animation.o \
	design_random_pixels.o   \
	design_text.o            \
	design_pong.o            \
	design_life.o            \
	design_cm5.o             \
	design_text_arcade.o

.PHONY: all
all: $(PROGRAM)

pixelwall: $(OBJECTS)
clean:
	rm -f $(PROGRAM) $(OBJECTS)
	rm -fr *.dSYM
