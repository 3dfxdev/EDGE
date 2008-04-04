#
# Makefile for Unix/Linux
#
# (Note: requires GNU make)
#

# --- variables that can be overridden ---

PROGRAM=qmipwad

CXX=g++

#OPTIM=-O2 -NDEBUG
OPTIM=-g3 -O

CPPFLAGS=-I.

CXXFLAGS=-Wall $(OPTIM) -fno-rtti -DUNIX

LDFLAGS=-Xlinker --warn-common $(SDL_LDFLAGS)


# ----- OBJECTS and LIBRARIES ------------------------------------------

OBJS=	u_assert.o \
	u_file.o   \
	u_util.o   \
	im_image.o \
	im_mip.o   \
	im_png.o   \
	pakfile.o  \
	main.o

LIBS=-lm -lpng


# ----- TARGETS ------------------------------------------------------

all: $(PROGRAM)

clean:
	rm -f $(PROGRAM) *.o core core.* ERRS

relink:
	rm -f $(PROGRAM)
	$(MAKE) $(PROGRAM)

$(PROGRAM): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(OBJS) -o $(PROGRAM) $(LIBS)


.PHONY: all clean relink

#--- editor settings ------------
# vi:ts=8:sw=8:noexpandtab
