#------------------------------------------------------------------------
# MP_SERVER
#------------------------------------------------------------------------
#
# Makefile for Linux/FLTK.
#

PROGNAME=mp-server

FLTK_CFLAGS=-I/usr/local/lib
FLTK_LIBS=-lfltk_images -lfltk -lX11 -lXext -lpng -ljpeg

CC=gcc
CXX=g++
CFLAGS=-O -g -Wall -DLINUX $(FLTK_CFLAGS)
CXXFLAGS=$(CFLAGS)
LDFLAGS=-L/usr/X11R6/lib -Lhawknl1.68/src
LIBS=-lm -lpthread -lNL $(FLTK_LIBS)

OBJS=autolock.o  \
     buffer.o    \
     client.o    \
     game.o      \
	 lib_argv.o  \
	 lib_list.o  \
	 lib_util.o  \
	 mp_main.o   \
	 network.o   \
	 packet.o    \
	 protocol.o  \
	 sys_assert.o \
	 sys_debug.o \
	 ticstore.o  \
     ui_log.o    \
     ui_menu.o   \
	 ui_stats.o  \
	 ui_window.o


# ----- TARGETS ------------------------------------------------------

all:    $(PROGNAME)

clean:
	rm -f $(PROGNAME) *.o core core.* mp_debug.txt ERRS update.log

$(PROGNAME): $(OBJS)
	$(CXX) $(CFLAGS) $(OBJS) -o $(PROGNAME) $(LDFLAGS) $(LIBS)

bin: all
	strip --strip-unneeded $(PROGNAME)

install: bin
	cp $(PROGNAME) /usr/bin/$(PROGNAME)
	chown root /usr/bin/$(PROGNAME)
	chmod 755 /usr/bin/$(PROGNAME)

.PHONY: all clean bin install

