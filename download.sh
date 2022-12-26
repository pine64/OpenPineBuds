#! /bin/bash

num=$(ls -l /dev/ttyUSB* | rev | cut -c 1)
#num=3
echo com is:$num
sudo -S dldtool -c $num -f out/open_source/open_source.bin
sudo minicom port$num
