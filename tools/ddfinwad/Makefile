#
# Linux MAKEFILE
#

PROGRAM=ddfinwad

CC=gcc
CFLAGS=-g -O -Wall
LDFLAGS=-Xlinker --warn-common
DEFINES=-DLINUX
LIBS=-lm

OBJS=wad_io.o main.o

all: $(PROGRAM)

clean:
	rm -f $(PROGRAM) $(OBJS) core

PHONY: all clean

#
# Main Source Compilation
# ^^^^^^^^^^^^^^^^^^^^^^^

%.o: %.c
	$(CC) $(CFLAGS) $(DEFINES) -c $< -o $@

$(PROGRAM): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@ $(LIBS)

