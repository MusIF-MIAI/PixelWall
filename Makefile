# Variabili
CC       ?= arm-linux-gnueabihf-gcc
CFLAGS   ?= -O2 -Wall
LDFLAGS  ?=
SYSROOT  ?= ./sysroot
TARGET   ?= snake_animation

# Percorsi
INCLUDES ?= -I$(SYSROOT)/usr/include
LIBS     ?= -L$(SYSROOT)/usr/lib -L$(SYSROOT)/lib -l:raylib.a -ldrm -lgbm -lasound -lpthread -lm

# Regole
all: $(TARGET)

$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) --sysroot=$(SYSROOT) $(INCLUDES) -o $(TARGET) $(TARGET).c $(LIBS) $(LDFLAGS)

clean:
	rm -f $(TARGET)

