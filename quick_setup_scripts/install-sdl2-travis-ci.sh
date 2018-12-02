#!/bin/sh
set -ex
wget https://www.libsdl.org/release/SDL2-2.0.9.tar.gz
tar -xzvf SDL2-2.0.9.tar.gz
cd SDL2-2.0.9 && ./configure --prefix=/usr && make && sudo make install