CC = gcc
CFLAGS ?= -Iinclude -I/usr/include/libdrm -W -Wall -Wextra -g -O2 -std=c11
LDFLAGS	?=
LIBS	:= -lGLESv2 -lglfw -lEGL

%.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $<

all: glDmaTexture

glDmaTexture: glDmaTexture.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

clean:
	-rm -f *.o
	-rm -f glDmaTexture
