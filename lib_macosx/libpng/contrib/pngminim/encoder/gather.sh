#!/bin/sh

cp ../../pngminus/pnm2png.c pnm2pngm.c
cp ../../../*.h .
cp ../../../*.c .
rm -f example.c pngtest.c pngpread.c pngr*.c pnggccrd.c pngvcrd.c
# Change the next 2 lines if zlib is somewhere else.
cp ../../../../zlib/*.h .
cp ../../../../zlib/*.c .
rm inf*.[ch]
rm uncompr.c minigzip.c example.c gz*
