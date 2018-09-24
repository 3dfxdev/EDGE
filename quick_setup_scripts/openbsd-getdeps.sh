#!/usr/local/bin/bash
if [ $USER != 'root' ]
	then
		echo Not running as root!
		echo Please \"sudo\" or \"su -\" to run this command.
		exit
fi

echo "This tool was designed for OpenBSD 6.2, but may work on other OpenBSD"
echo "based distributions using 'pkg_add' and are of similar age."
echo ""
echo "This tool will automatically download and install all the libraries"
echo "that 3DGE depends on."
echo ""
read -p "Do you wish to do this? " -r REPLY
echo ""

if [ $REPLY == y ]
then
	pkg_add -v sdl2 physfs sdl2-net \
		libogg libvorbis \
		cmake git
fi

