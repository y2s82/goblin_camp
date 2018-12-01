#/usr/bin/env sh

#
#Add Library
#
#By Tony Sim (y2s82)
#
#Add STL library based on Boost couterpart object
#This is a backup way to update library when removing boost
#It should not be used unless boostrm.sh massed up
#

OBJ=$1
TEMP=temp.temp
for file in $(git grep -E $OBJ | sed -ne 's/^\([^:]*\):.*$/\1/p' | sort | uniq)
do
    if [[ $(echo $file | cut -d'.' -f2) = "hpp" ]]
    then
        sed -e "s/\(^.*#pragma once.*$\)/\1\n#include<memory>/" $file > $TEMP
    elif [[ $(echo $file | cut -d'.' -f2) = "cpp" ]]
    then
        sed -e "s/\(^.*http:.*$\)/\1\n#include<memory>/" $file > $TEMP
    fi
    mv $TEMP $file
done
