#!/bin/bash
if [ "$EUID" -ne 0 ]
  then echo "Please run as root"
  exit
fi
add-apt-repository ppa:jonathonf/ffmpeg-4 -y
apt-get install ffmpeg -y
apt-get install libsdl2-2.0 -y
apt-get install libcurl4 -y
apt-get install libsdl2-ttf-dev -y
apt-get install libpthread-stubs0-dev -y
apt-get install libavformat57 -y
#etc
