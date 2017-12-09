#!/bin/bash
#-*- coding: iso-8859-15 -*-
# INDUINO METEOSTATION
# http://induino.wordpress.com 
# 
# NACHO MAS 2013
# MAGNUS W. ERIKSEN 2017

source meteoconfig.py

if [[ "$INDISERVER" = "localhost" && "$INDITUNNEL" = "false" ]]
then
    eval $KILLEXEC
elif [[ "$INDISERVER" = "localhost" && "$INDITUNNEL" = "true" && "$INDISTARTREMOTE" = "true" ]]
then
    eval $REMOTEKILLEXEC
elif [[ "$INDISERVER" = "localhost" && "$INDITUNNEL" = "true" && "$INDISTARTREMOTE" = "false" ]]
then
    eval $SSHEXIT
fi

unset IFS

killall meteoRRD_updater.py
killall meteoRRD_graph.py
killall sounding.py
killall meteoRRD_MaxMinAvg.py
