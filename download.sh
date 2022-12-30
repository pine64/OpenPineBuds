#! /bin/bash

num=$(ls -l /dev/ttyACM* | rev | cut -c 1)
echo com is:$num
splitPorts=($num)
echo "This tool assumes your buds are the only thing connected and are enumerated {right,left} order. YMMV"
echo "Right bud is at ${splitPorts[0]}"
echo "Left bud is at ${splitPorts[1]}"

echo "Please disconnect and reconnect the bud on the right"

bestool write-image out/open_source/open_source.bin --port /dev/ttyACM${splitPorts[0]}

echo "Please disconnect and reconnect the bud on the left"
bestool write-image out/open_source/open_source.bin --port /dev/ttyACM${splitPorts[1]}
