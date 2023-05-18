#!/usr/bin/env sh

num=$(find /dev -name 'ttyUSB*' | rev | cut -c 1)
echo "$num"
sudo minicom "port$num"
