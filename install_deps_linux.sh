#!/bin/bash
old_dir=$PWD
curl -o sdl.tar.gz http://hg.libsdl.org/SDL/archive/007dfe83abf8.tar.gz
tar -xf sdl.tar.gz
cd SDL-007dfe83abf8/
mkdir -p build
cd build
../configure
make
sudo make install
cd $old_dir 
curl https://github.com/libtcod/libtcod/archive/1.9.0.zip -L -o libtcod-1.9.0.zip
unzip libtcod-1.9.0.zip
cd libtcod-1.9.0/build/autotools
autoreconf -i
./configure CFLAGS='-O2'
make
sudo make install
cd $old_dir 
