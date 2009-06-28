
EXES = coal

CC=g++
CFLAGS=-Wall -O0 -g3

#==============================================================================

all: $(EXES)

clean:
	rm -f *.o *.obj $(EXES) core


#==============================================================================

QCCFILES = coal.o
coal: $(QCCFILES)
	$(CC) $(CFLAGS) -o $@ $(QCCFILES) -lm

test:
	./coal mod_base.ec mod_hud.ec mod_player.ec doom_hud.ec

.PHONY: all clean test

#--- editor settings ------------
# vi:ts=8:sw=8:noexpandtab
