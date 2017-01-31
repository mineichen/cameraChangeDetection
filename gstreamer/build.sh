#!/bin/sh
pkg=gstreamer-video-1.0
gstreplace=gstmiunCameraChangeDetector


oldDir=$PWD
cd $(dirname "$0")

#g++ -fPIC $CPPFLAGS -I lib $(pkg-config --cflags gstreamer-1.0 $pkg) -c -o $gstreplace.o $gstreplace.c 
mkdir -p build
rm -rf build/*

gcc -Wall -Werror -fPIC $CPPFLAGS -I lib $(pkg-config --cflags gstreamer-1.0 $pkg) -c -o build/$gstreplace.o $gstreplace.c 
if test $? -ne 0; then
    exit 1
fi

gcc -shared -o build/$gstreplace.so build/$gstreplace.o lib/harris.a $(pkg-config --libs gstreamer-1.0 $pkg)
if test $? -ne 0; then
    exit 1
fi

cd $OLDDIR
echo "Success!"