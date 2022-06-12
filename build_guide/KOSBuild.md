# EDGE Windows KOS Environment Setup for Dreamcast
### (C) 2011-2022 The EDGE Team & Contributors
#### EDGE is Licensed under the GPLv2 (and greater)
http://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html
##### This is a WORK IN PROGRESS. Last revised 6/11/22 ~CA
------------


## Setting up KOS, WSL2, and EDGE (Windows 10/11)
KOS is easy to get up and running on a variety of platforms, but on Windows this always required setting up Cygwin. But, with the addition of the Windows Subsystem for Linux (WSL2) in Windows 10/11, one can now run Ubuntu (in our case, Debian) and the Bash shell natively in Windows, and that means one can easily setup KOS, the SH4 toolchain and (almost) everything else required to develop and deploy apps to a Sega Dreamcast. 

The following is a short build guide which is designed to help set up EDGE in the KOS environment (originally adapted from [here](https://brianpeek.com/dreamcast-gamedev-with-kos-and-wsl/ "here")).

## Requirements
- A Sega Dreamcast capable of booting MIL-CDs…this is any machine manufactured prior to October 2000.

- A Dreamcast Broadband Adapter (HIT-0300 or HIT-0400 both work) - if you are developing for emulation you do not need the broadband adapter 

- The Dreamcast plugged into your network via Ethernet

- The Linux Subsystem for Windows (WSL2) installed and configured (in this guide, we use and recommend the Debian distro, which can be obtained via the Microsoft Store)

## Updating LXSS
Make sure you upgrade the subsystem by issuing these two commands:

```bash
sudo apt-get update
sudo apt-get upgrade
```

## Downloading and Building the Toolchain and KOS
Once completed, you will need to install a few packages required to retrieve and build the SDK and tools:

#### Setup pre-reqs for SDK
```bash
sudo apt-get -y install git make gcc texinfo libjpeg-dev libpng-dev subversion genromfs libelf-dev python curl wget cmake
```
#### Download KOS and KOS-PORTS
Now that the appropriate packages are installed, make sure you are in your home directory and then download the KOS repos:

```bash
git clone https://github.com/KallistiOS/KallistiOS.git kos
git clone --recursive https://github.com/KallistiOS/kos-ports.git
```
Open up Windows Explorer and navigate to the Debian home directory and into kos\utils\dc-chain. Copy "config.mk.stable.sample", and rename it to simply "config.mk". Not doing so will throw errors when you attempt to run the next steps.

#### Setup KOS toolchain
Next, you need to download and unpack the toolchain bits used to build the Dreamcast toolchain:

```bash
cd ~/kos/utils/dc-chain/
./download.sh
./unpack.sh
```

NOTE: The unpacking process will not display any output and may take a long time to complete.

### Building Toolchain
With the packages unpacked, you can now build the toolchain. This process builds a compatible compiler (gcc) and other tools for the Dreamcast’s SH4 CPU. While in the same directory as above, run:

```bash
sudo make erase=1
```
**This will take an extremely long time to complete! We recommend adding -j2 or -j4 to speed this process up.**

### Setting up Terminal Environment for KOS (environ.sh)

With that complete, you now need to setup your Bash environment so the SDK and tools can be found and used. 

First, copy the following file and change its permissions:

```bash
cp ~/kos/doc/environ.sh.sample ~/kos/environ.sh
chmod 777 ~/kos/environ.sh
```

Next, edit the environ.sh file we just copied to point it at the tool location. You can use Nano, or simply open up Windows Explorer and use native Notepad to handle this:

```bash
nano ~/kos/environ.sh
```

Look for the entry exporting **KOS_BASE** and set it to the following:

```shell
/home/YOUR_USERNAME/kos
```
Obviously, replace YOUR_USERNAME with your WSL2 username (example below:

```shell
export KOS_BASE="/home/corbin/kos"

```

#### Setup DC Load IP (if applicable, not needed for Emulation)
Next, look for the entry exporting KOS_LOADER and add the following prior to the -x argument:

```shell
-t IP_ADDRESS
```
…where IP_ADDRESS is a valid, unused IP address on your network. Mine looks like this:

```shell
export KOS_LOADER="dc-tool -t 192.168.2.250 -x"
```
NOTE: Make sure the IP is valid to your subnet.

#### Edit Bash.RC

Finally, edit the **.bashrc** file in your home directory:

```bash
nano ~/.bashrc
```

…and at the very bottom, add this line so the environ.sh file you just edited gets run at login:

```shell
. ~/kos/environ.sh
```

Note: Do not forget the period as the first character!

#### Restart Debian (WSL)
Now, exit and restart Debian. To make sure the environment is setup, use the printenv command and you should see a variety of environment variables listed starting with KOS_ listed.

## Building KOS and KOS-PORTS
Next, KOS itself needs to be built. To do this, change into the kos directory and run the make command:

```bash
cd ~/kos
make
```

Once completed, build the kos-ports project:

```bash
cd ~/kos-ports/utils/
./build-all.sh
```

And finally, build the KOS example projects:

```bash
cd ~/kos/examples
make
```

Note: A few of these examples may not build but don’t worry about that for now.

#### Downloading and Building dcload-ip
Next up, you will need to download and build the dcload-ip tool. This is what will allow you to deploy a game to a retail Dreamcast over the Broadband Adapter (BBA). Change back to your home directory, run the following:

```bash
cd ~
git clone git://git.code.sf.net/p/cadcdev/dcload-ip
cd dcload-ip
sudo make install
```

You may see an error about /usr/local/bin existing on the final step, which is safe to ignore.

Now, you will need to burn the host dcload program to a CD so we can run it on the physical Dreamcast machine and upload our binary. Because WSL2 doesn’t yet have hardware support, it can’t talk to a CD-R drive. But, there is an optional quick package that contains the required tools and the cygwin libraries to run them. 

You can download that package here:

[dcload-disc.zip (1.42 mb)](https://brianpeek.com/downloads/projects/dcload-disc.zip "dcload-disc.zip (1.42 mb)")

Unzip this somewhere on your machine, then open an admin Windows command prompt (not Bash) and navigate to the files. Then, run the following:

```bash
wodim -scanbus
```
This will return some information about every drive it finds. In the output, look for your recordable drive in the list and make a note of the 3 comma-delimited digits that identify the bus and location of the drive. Mine looks like this:

> scsibus0:  
        0,0,0     0) 'HL-DT-ST' 'BD-RE  WH16NS40 ' '1.02' Removable CD-ROM
        0,1,0     1) *  
        0,2,0     2) *  
        0,3,0     3) *  
        0,4,0     4) *  
        0,5,0     5) *  
        0,6,0     6) *  
        0,7,0     7) HOST ADAPTOR

For these next steps, replace the X,X,X with your values.

Insert a blank CD-R into your drive and burn the first part of the disc, the audio track, as follows:

```bash
wodim dev=X,X,X -multi -audio -tao audio.raw
```

Now, burn the data track:

```bash
wodim dev=X,X,X -xa -tao -multi data.raw
```

With those two items burned, you now be able to eject the CD, pop it in your Dreamcast, and it will run.

#### Running a Sample
With dcload-ip running on your Dreamcast, you should see a MAC address displayed.

We need to tell our Windows machine how to find that device by an IP address. If you recall, earlier you selected an IP for your Dreamcast. You will need that here.

Open an admin command prompt (not Bash, but standard command prompt) and run the arp command as follows:

```bash
arp -s IP_ADDRESS MAC_ADDRESS
```
Replace IP_ADDRESS with the one selected above, and MAC_ADDRESS with the one seen on screen, but use hyphens instead of colons. Here’s an example:

```bash
arp -s 192.168.2.250 00-d0-f1-02-3f-07
```
**This command must be run after every reboot; it will not be saved by Windows.**

You may need to exit and restart Debian after running this command so it will know about the change.

## Downloading EDGE

Clone EDGE into your home directory:

git clone https://github.com/3dfxdev/EDGE.git dreamedge

Next, you need to setup and install a few customary libraries that EDGE depends on that are not part of kos-ports. These are in the directories below:

(WIP WIP)
