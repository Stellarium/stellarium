#!/bin/bash
#-*- coding: iso-8859-15 -*-
# INDUINO METEOSTATION
# http://induino.wordpress.com 
# 
# NACHO MAS 2013
# MAGNUS W. ERIKSEN 2017
# Startup script

source meteoconfig.py

if [[ "$INDISERVER" = "localhost" && "$INDITUNNEL" = "false" ]]
then
    eval $INDILOCALEXEC &
elif [[ "$INDISERVER" = "localhost" && "$INDITUNNEL" = "true" && "$INDISTARTREMOTE" = "true" ]]
then
    eval $INDIREMOTEFORKEXEC
elif [[ "$INDISERVER" = "localhost" && "$INDITUNNEL" = "true" && "$INDISTARTREMOTE" = "false" ]]
then
    eval $INDIREMOTEEXEC
    eval $SSHCHECK
fi

unset IFS

if [ -f "meteo.rrd" ];
then
   echo "RRD file exists."
else
   echo "RRD file exists does not exist. Creating"
   ./meteoRRD_createRRD.py $EXECNOOUTPUT
fi
./meteoRRD_updater.py $EXECNOOUTPUT &
./meteoRRD_graph.py $EXECNOOUTPUT &
./sounding.py $EXECNOOUTPUT &
./meteoRRD_MaxMinAvg.py $EXECNOOUTPUT &

