#
# DEH_EDGE Plugin
# < MacOS X MAKEFILE >
#

OUTNAME=libdehedge.a

CXX=g++

CXXFLAGS=-O -Wall
LDFLAGS=
DEFINES=-DMACOSX -DDEH_EDGE_PLUGIN
LIBS=-lm

OBJS=convert.o main.o system.o util.o wad.o info.o mobj.o \
     sounds.o frames.o things.o attacks.o weapons.o ammo.o \
	 misc.o text.o storage.o patch.o rscript.o buffer.o

all: $(OUTNAME)

clean:
	rm -f $(OUTNAME) $(OBJS) ERRS core core.* deh_debug.log

.PHONY: all clean

#
# Main Source Compilation
#

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(DEFINES) -c $< -o $@

$(OUTNAME): $(OBJS)
	libtool -static -o $(OUTNAME) - $(OBJS)
	ranlib $(OUTNAME)

