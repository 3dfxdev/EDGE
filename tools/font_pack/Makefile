#
# FONT_PACK Makefile for Linux
#
# (Note: requires GNU make)
#

# --- variables that can be overridden ---

PROGRAM=font_pack

CXX=g++

OPTIM=-g3 -O

# operating system choices: linux [w32]
OS=linux


# --- internal stuff from here on ------

linux_FLAGS=-DLINUX
win32_FLAGS=-DWIN32

TTF_FLAGS=-I/usr/include/freetype2
TTF_LIBS=-lfreetype


# --- putting it all together ---------

CPPFLAGS=-I.

CXXFLAGS=-Wall $(OPTIM) $($(OS)_FLAGS) $(TTF_FLAGS)

LDFLAGS=-Xlinker --warn-common


# ----- OBJECTS and LIBRARIES ------------------------------------------

OBJS=main.o

LIBS=-lm $(TTF_LIBS)


# ----- TARGETS ------------------------------------------------------

all: $(PROGRAM)

clean:
	rm -f $(PROGRAM) *.o core core.* ERRS

$(PROGRAM): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(OBJS) -o $(PROGRAM) $(LIBS)

.PHONY: all clean

