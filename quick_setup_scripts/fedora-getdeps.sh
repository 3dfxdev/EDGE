#!/bin/bash
if [ $UID != 0 ]
	then
		echo Not running as root!
		echo Please \"sudo\" or \"su -\" to run this command.
		exit
fi

echo "This tool was designed for Fedora 27, but may work on other Fedora"
echo "based distributions using dnf and are of similar age."
echo ""
echo "This tool will automatically download and install all the libraries"
echo "that 3DGE depends on."
echo ""
read -p "Do you wish to do this? " -n 1 -r
echo ""

if [[ $REPLY =~ ^[Yy]$ ]]
then
	dnf install -y SDL2-devel physfs-devel SDL2_net-devel glew-devel libpng-devel libogg-devel libvorbis-devel ghc-zlib-devel libjpeg-turbo-devel \
		cmake cmake-gui mesa-libGL-devel gcc-c++ make git
fi

