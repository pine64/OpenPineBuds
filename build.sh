#!/bin/bash
make -j T=open_source DEBUG=1 >log.txt 2>&1

if [ $? -eq 0 ]; then
	echo "build success"
else
	echo "build failed and call log.txt"
	cat log.txt | grep "error:*"
fi
