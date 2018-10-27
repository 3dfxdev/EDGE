#!/bin/sh
set -ex

mkdir physfs/build
cd physfs/build
git clone https://github.com/Didstopia/physfs
cmake .. -DCMAKE_BUILD_TYPE=Release -DPHYSFS_BUILD_STATIC=1