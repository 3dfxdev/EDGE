
CFLAGS=-O2 -Wall -I. `sdl-config --cflags`

EXE1=roqtoppm
LDFLAGS1=-O2
LIBS1=
OBJS1=roqtoppm.o roq_read.o

EXE2=sroqplay
LDFLAGS2=-O2
LIBS2=`sdl-config --libs`
OBJS2=sroqplay.o roq_read.o

CC=gcc

all: $(EXE1) $(EXE2)

.c.o:
	$(CC) -c $(CFLAGS) -o $*.o $*.c

$(EXE1): $(OBJS1)
	$(CC) $(LDFLAGS1) -o $(EXE1) $(OBJS1) $(LIBS1)

$(EXE2): $(OBJS2)
	$(CC) $(LDFLAGS2) -o $(EXE2) $(OBJS2) $(LIBS2)

clean:
	rm -f *.o $(EXE1) $(EXE2)

$(OBJS1): roq.h
$(OBJS2): roq.h
