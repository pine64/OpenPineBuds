#!/usr/bin/env sh

make -j "$(nproc)" T=open_source DEBUG=1 clean
