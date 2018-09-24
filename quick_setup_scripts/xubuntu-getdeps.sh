#!/bin/bash
if [ $UID != 0 ]
	then
		echo Not running as root!
		echo Please \"sudo\" or \"su -\" to run this command.
		exit
fi

echo "This tool was designed for Xubuntu 18.04 but may work on other Canonical"
echo "based distributions using 'apt-get', or 'aptitude'..."
echo ""
echo "This tool will automatically download and install all the libraries"
echo "that 3DGE depends on."
echo ""
read -p "Do you wish to do this? If not, please close the window. " -r REPLY
echo ""

if [ $REPLY  ]
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

