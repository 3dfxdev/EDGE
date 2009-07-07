
OUTPUTS=libcoal.a burner

CXX=g++
CXXFLAGS=-Wall -O0 -g3
AR=ar rcs

#==============================================================================

all: $(OUTPUTS)

clean:
	rm -f *.o *.obj $(OUTPUTS) core

test:
	./burner mod_base.ec mod_hud.ec mod_player.ec doom_hud.ec

.PHONY: all clean test

#==============================================================================

COALFIRES=c_compile.o c_execute.o

libcoal.a: $(COALFIRES)
	$(AR) $@ $(COALFIRES)

burner: burner.o libcoal.a
	$(CXX) $(CXXFLAGS) -o $@ burner.cc libcoal.a -lm

#--- editor settings ------------
# vi:ts=8:sw=8:noexpandtab
