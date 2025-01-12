# Variabili
CC       ?= arm-linux-gnueabihf-gcc
CFLAGS   ?= -O2 -Wall
LDFLAGS  ?=
SYSROOT  ?= ../sysroot
TARGET   ?= snake_animation

# Percorsi
INCLUDES ?=  -I raylib/src/ -nostdinc -I ../sysroot/usr/include -I . -I ../sysroot/usr/lib/gcc/arm-linux-gnueabihf/8/include  -I ../sysroot/usr/include/arm-linux-gnueabihf/ -I ../sysroot/usr/include/libdrm -I ../sysroot/usr/lib/gcc/arm-linux-gnueabihf/8/include-fixed/
LIBS     ?= -L$(SYSROOT)/usr/lib -L$(SYSROOT)/lib  -lraylib -lGL -lm -ldrm -lgbm -lEGL  -L ../raylib/src/ -o snake

# Regole
all: $(TARGET)

$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) --sysroot=$(SYSROOT) $(INCLUDES) -o $(TARGET) $(TARGET).c $(LIBS) $(LDFLAGS)

clean:
	rm -f $(TARGET)

