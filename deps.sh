#!/bin/bash

MACH_ARCH=`uname -m`

sudo apt-get update
sudo apt-get -y upgrade

sudo apt-get install -y qt5-default qtcreator libasound2-dev build-essential libopencv-dev
sudo apt-get install -y libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libgstreamer-plugins-bad1.0-dev gstreamer1.0-libav libssl-dev openssl

if [ $MACH_ARCH = 'armv7l' ] ; then
sudo pip3 install --upgrade pip3
sudo pip3 install --upgrade bme680
fi


