#!/usr/bin/env sh

#
#Boost Remove
#
#By Tony Sim (y2s82)
#
#Requires: git grep mv sed sort uniq compatible-shell
#
#This is a dumb script that takes one argument: bash objects to change into its std:: counterpart
#It will search for the object in all source code, and replace boost:: into std::
#

if [[ $# != 2 ]] 
then
    echo "USAGE: ./boostrm <object-name-or-extended-regex> <library-name>"
    echo "The first argument is used by git grep with extended regular expression to find the boost libraries" 
    echo "The second argument is for the #include<library-name> that should be added to get the std:: counterpart to run"
    echo "E.g. ./boostrm weak_ptr memory"
    echo "E.g. ./boostrm '(weak|shared)_ptr' memory"
    exit 1
fi

OBJ=$(echo $1 | sed "s/boost:://") #filter the 1st argument to contain just the name of the object without a tag
INC_LIB=$2 #the STL for the std:: version
TEMP=temp.$RANDOM.$(date) #temporary file to store the changes


for file in $(git grep -E $OBJ | sed -ne 's/^\([^:]*\):.*$/\1/p' | sort | uniq)
do
    sed\
    -e "s%^.*#include.*<.*boost/$OBJ.hpp>.*$%#include <$INC_LIB>%"\ #replace the boost library with the STL library specified
    -e "s/boost::$OBJ/std::$OBJ/g"\                                 #replace the boost:: tags with std:: tags for the given object specified
    $file\
    > $TEMP

    mv $TEMP $file
done
