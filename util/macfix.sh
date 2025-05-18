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

MD4C_FILE=Stellarium.app/Contents/Frameworks/libmd4c-html.0.dylib
if [ -f $MD4C_FILE ]; then
    for L in $(otool -L $MD4C_FILE | awk '{print $1}')
    do
        if [[ "$L" == *@rpath* ]]
        then
            FWPATH=$(echo $L | sed 's,@rpath,@executable_path/../Frameworks,g')
            install_name_tool -change $L $FWPATH $MD4C_FILE
        fi
    done
fi