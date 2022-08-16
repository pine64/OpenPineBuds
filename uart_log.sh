#! /bin/bash

num=$(ls -l /dev/ttyUSB* | rev | cut -c 1)
#num=1
echo $num
sudo minicom port$num
