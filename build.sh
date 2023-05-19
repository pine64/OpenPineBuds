#!/usr/bin/env sh

if make -j "$(nproc)" T=open_source DEBUG=1 >log.txt 2>&1; then
	echo "build success"
else
	echo "build failed and call log.txt"
	grep "error:" log.txt
fi
