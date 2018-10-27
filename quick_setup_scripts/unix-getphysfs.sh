#!/bin/sh
set -ex

cd $0/../..
git clone https://github.com/Didstopia/physfs
mkdir physfs/build
cd physfs/build
cmake .. -DCMAKE_BUILD_TYPE=Release -DPHYSFS_BUILD_STATIC=1
