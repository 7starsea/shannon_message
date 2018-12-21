#!/bin/bash


cmake=`which cmake`
if [ -z $cmake ]
then 
    echo "cmake does not exist!"
    exit -1
fi 


curr_path=`pwd`
ConfigFile=$curr_path/env-config.cmake


[ -d build ] || mkdir build
cd build
    [ -d release ] || mkdir release
    cd release
        
        $cmake -C $ConfigFile -DCMAKE_BUILD_TYPE=Release $curr_path
        make 
    
    cd ..  # # release
    
cd ..  # # build

