#
# Makefile for Linux
#
EPI_DIR=../../edge/epi
PROGNAME=upgrader

CC=g++
CFLAGS=-O3 -Wall -DLINUX -DUSE_DL_PREFIX
CXXFLAGS=-O3 -Wall -DLINUX -DUSE_DL_PREFIX -I$(EPI_DIR)
LDFLAGS=-Xlinker --warn-common

LIBS=$(EPI_DIR)/libepi.a

OBJS=main.o

# ----- TARGETS ------------------------------------------------------

all: $(PROGNAME)

clean:
	rm -f $(PROGNAME) *.o

$(PROGNAME): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $(PROGNAME) $(LIBS)

.PHONY: all clean
