
EXES = coal

CC=gcc
CFLAGS=-Wall -O0 -g3

#==============================================================================

all: $(EXES)

clean:
	rm -f *.o *.obj $(EXES) core


#==============================================================================

QCCFILES = coal.o
coal: $(QCCFILES)
	$(CC) $(CFLAGS) -o $@ $(QCCFILES)


#--- editor settings ------------
# vi:ts=8:sw=8:noexpandtab
