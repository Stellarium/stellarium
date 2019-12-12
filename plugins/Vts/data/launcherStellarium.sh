#!/bin/sh
# launcherStellarium.sh

# Input parameter $1 is the path to the VTS project file
# Input parameter $2 is the client application ID
# Input parameter --serverport is optionnal (8888 by default)

# Default server port 
serverport=8888
vtsconfig="$1"
appid=$2
shift && shift

while [ "$1" != "" ]; do
	case "$1" in
		"--serverport")
			serverport=$2
			shift
			;;
		"--appid")
			appid=$2
			shift
			;;
	esac
	shift
done

echo $appid $vtsconfig $appid --serverport $serverport --config-file $VTS_WORKING_DIR/config.ini --appid $appid
