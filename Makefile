# Variabili
CC       ?= arm-linux-gnueabihf-gcc
CFLAGS   ?= -O2 -Wall
LDFLAGS  ?=
SYSROOT  ?= ../sysroot
TARGET   ?= snake_animation

# Percorsi
INCLUDES ?= -I$(SYSROOT)/usr/include  -I raylib/src/
LIBS     ?= -L$(SYSROOT)/usr/lib -L$(SYSROOT)/lib  -lraylib -lGL -lm -ldrm -lgbm -lEGL  -L ../raylib/src/ -o snake

# Regole
all: $(TARGET)

$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) --sysroot=$(SYSROOT) $(INCLUDES) -o $(TARGET) $(TARGET).c $(LIBS) $(LDFLAGS)

clean:
	rm -f $(TARGET)

