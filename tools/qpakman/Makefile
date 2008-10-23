#
# Makefile for Unix/Linux
#
# (Note: requires GNU make)
#

# --- variables that can be overridden ---

PROGRAM=qpakman

CXX=g++

OPTIM=-g -O2

CPPFLAGS=-I.
CXXFLAGS=-Wall $(OPTIM) -fno-rtti -DUNIX

LDFLAGS=-Xlinker --warn-common


# ----- OBJECTS and LIBRARIES ------------------------------------------

OBJS=	u_assert.o \
	u_file.o   \
	u_util.o   \
	im_color.o \
	im_image.o \
	im_gen.o   \
	im_mip.o   \
	im_png.o   \
	im_tex.o   \
	archive.o  \
	arc_spec.o \
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
