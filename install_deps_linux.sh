#!/bin/bash
old_dir=$PWD
curl -L -o libtcod-1.5.1.tar.gz https://bitbucket.org/libtcod/libtcod/downloads/libtcod-1.5.1-linux64.tar.gz
tar -xf libtcod-1.5.1.tar.gz
cd libtcod-1.5.1/
make -f makefiles/makefile-linux64
#sudo cp -r include/* /usr/include/libtcod
#sudo cp libtcod*.so /usr/lib/
cd $old_dir 
