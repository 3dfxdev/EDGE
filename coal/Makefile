
OUTPUTS=libcoal.a burner

CXX=g++
CXXFLAGS=-Wall -O0 -g3

#==============================================================================

all: $(OUTPUTS)

clean:
	rm -f *.o *.obj $(OUTPUTS) core

test:
	./burner mod_base.ec mod_hud.ec mod_player.ec doom_hud.ec

.PHONY: all clean test

#==============================================================================

QCCFILES=c_compile.o c_execute.o

libcoal.a: $(QCCFILES)
	$(CXX) $(CXXFLAGS) -o $@

burner: burner.o libcoal.a
	$(CXX) $(CXXFLAGS) -o $@ burner.cc -lcoal -lm

#--- editor settings ------------
# vi:ts=8:sw=8:noexpandtab
