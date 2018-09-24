#!/bin/bash
if [ $UID != 0 ]
	then
		echo Not running as root!
		echo Please \"sudo\" or \"su -\" to run this command.
		exit
fi

echo "This tool was designed for Ubuntu 16.04, but may work on other Debian"
echo "based distributions using apt-get and are of similar age."
echo ""
echo "This tool will automatically download and install all the libraries"
echo "that 3DGE depends on."
echo ""
echo "Until the script for libphysfs-dev is updated to 3.1.0, we have to"
echo "manually pull and build physfs. This will install the library."
echo ""
read -p "Do you wish to do this? " -n 1 -r
echo ""

if [[ $REPLY =~ ^[Yy]$ ]]
then
	apt-get install -y libsdl2-dev libsdl2-net-dev libogg-dev libvorbis-dev libghc-zlib-dev \
		cmake cmake-gui libgl1-mesa-dev g++ make git zlib1g-dev
	mkdir physfs/build
	cd physfs/build
	git clone https://github.com/criptych/physfs
	cmake .. -DCMAKE_BUILD_TYPE=Release -DPHYSFS_BUILD_STATIC=1
	#make
	#make install
	#cd ..
fi

