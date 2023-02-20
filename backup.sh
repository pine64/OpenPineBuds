#!/bin/bash

BKPPATH="firmware-backups"

mkdir -p "$BKPPATH"

NOW=$(date +%s)

num=$(find /dev -name 'ttyACM*' | sort | rev | cut -c 1)
echo com is: "$num"
mapfile -t splitPorts <<< "$num"
echo "This tool assumes your buds are the only thing connected and are enumerated {right,left} order. YMMV"
echo "Right bud is at ${splitPorts[0]}"
echo "Left bud is at ${splitPorts[1]}"

echo "Please disconnect and reconnect the bud on the right"
bestool read-image --port "/dev/ttyACM${splitPorts[0]}" "${BKPPATH}/firmware-${NOW}-${splitPorts[0]}.bin.bkp"

echo "Please disconnect and reconnect the bud on the left"
bestool read-image --port "/dev/ttyACM${splitPorts[1]}" "${BKPPATH}/firmware-${NOW}-${splitPorts[1]}.bin.bkp"
