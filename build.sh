#!/bin/sh
mkdir -p prebuild
if [ ! -e "prebuild/c-icap-modules-0.1.6" ]; then
	cd prebuild 
	apt-get source libc-icap-mod-urlcheck
	cd c-icap-modules-0.1.6 && ./configure && cd ..
	cd ..
fi

mkdir -p output && rm -rf ./output/*
gcc -fPIC -shared service/changyy/srv_changyy.c  -o output/changyy.so -Iprebuild/c-icap-modules-0.1.6 -DHAVE_CONFIG_H -I/usr/include/c_icap  -L/usr/local/lib/c_icap_modules
