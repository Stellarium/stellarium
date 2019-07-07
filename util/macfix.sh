#!/bin/bash

BIN_FILE=Stellarium.app/Contents/MacOS/stellarium
for P in $(otool -L $BIN_FILE | awk '{print $1}')
do 
    if [[ "$P" == *@rpath* ]] 
    then 
        QTFWPATH=$(echo $P | sed 's,@rpath,@executable_path/../Frameworks,g')
        install_name_tool -change $P $QTFWPATH $BIN_FILE
    fi 
done 
