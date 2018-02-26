#
# Makefile for UNIX/Linux
#

CC=gcc

PROGNAME=levnamer

CFLAGS=-O -g -Wall -DUNIX
LIBS=-lm

OBJS=main.o system.o wad.o

SRC_DIR=levnamer
SRC_FILES=$(SRC_DIR)/[Mm]ake* $(SRC_DIR)/*.[ch] $(SRC_DIR)/*.txt


# ----- TARGETS ------------------------------------------------------

all:    $(PROGNAME)

clean:
	rm -f $(PROGNAME) *.o core levnamer.txt levnamer.exe

$(PROGNAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(PROGNAME) $(LIBS)

backup: clean
	tar czf /tmp/levnamer.tgz make* *.[ch]

bin: all
	strip --strip-unneeded $(PROGNAME)
	cat README.txt > levnamer.txt

tar: clean
	cd ..; tar czf /tmp/levnmsrc.tgz $(SRC_FILES)

zip: clean
	cd ..; zip -l /tmp/levnmsrc.zip $(SRC_FILES)

.PHONY: all clean backup bin tar zip

