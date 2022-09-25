#! /bin/bash

num=$(ls -l /dev/ttyUSB* | rev | cut -c 1)
#num=3
echo com is:$num
#sudo -S dldtool -c $num -f out/2300_open_source/2300_open_source.bin
sudo -S dldtool -c $num -f out/mic_alg/mic_alg.bin
sudo minicom port$num

