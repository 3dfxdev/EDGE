#!/bin/sh
if [ $USER != 'root' ]
	then
		echo Not running as root!
		echo Please \"sudo\" or \"su -\" to run this command.
		exit
fi

echo "This tool was designed for FreeBSD 12.0-CURRENT, but may work on other FreeBSD"
echo "based distributions using 'pkg' and are of similar age."
echo ""
echo "This tool will automatically download and install all the libraries"
echo "that 3DGE depends on."
echo ""
read -p "Do you wish to do this? " -r REPLY
echo ""

if [ $REPLY == y ]
then
	pkg install -y sdl2 physfs-devel sdl2_net libogg libvorbis \
		cmake cmake-gui mesa-libs git
fi

