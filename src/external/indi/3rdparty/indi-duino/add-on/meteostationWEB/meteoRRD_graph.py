#!/usr/bin/python
#-*- coding: iso-8859-15 -*-
# INDUINO METEOSTATION
# http://induino.wordpress.com 
# 
# NACHO MAS 2013

import sys, os
import rrdtool
import time
from meteoconfig import *
import math

blue="0080F0"
orange="F08000"
red="FA2000"
white="AAAAAA"

preamble=["--width","600",
         "--height","300",
         "--color", "FONT#FF0000",
	 "--color", "SHADEA#00000000",
	 "--color", "SHADEB#00000000",
	 "--color", "BACK#00000000",
	 "--color", "CANVAS#00000000"]

def graphs(time):
	ret = rrdtool.graph( CHARTPATH+"temp"+str(time)+".png","--start","-"+str(time)+"h","-E",
          preamble,
	 "--title","Temperature",
	 "--vertical-label=Celsius ºC",
	 "DEF:T=meteo.rrd:T:AVERAGE",
	 "DEF:Tmax=meteo.rrd:T:MAX",
	 "DEF:Tmin=meteo.rrd:T:MIN",
	 "DEF:Dew=meteo.rrd:Dew:AVERAGE",
	 "LINE1:T#"+red+":Ambient Temperature",
	 "HRULE:0#00FFFFAA:ZERO",
	 "AREA:Dew#"+red+"40:Dew Point\\r",
	 "COMMENT:\\n",
	 "GPRINT:T:AVERAGE:Avg Temp\: %6.2lf %S\\r")

	ret = rrdtool.graph( CHARTPATH+"alltemp"+str(time)+".png","-A","--start","-"+str(time)+"h","-E",
          preamble,
	 "--title","Temperaturas",
	 "--vertical-label=Celsius ºC",
	 "DEF:IR=meteo.rrd:IR:AVERAGE",
	 "DEF:Thr=meteo.rrd:Thr:AVERAGE",
	 "DEF:Tp=meteo.rrd:Tp:AVERAGE",
	 "DEF:Tir=meteo.rrd:Tir:AVERAGE",
	 "DEF:Dew=meteo.rrd:Dew:AVERAGE",
	 "LINE1:IR#00F0F0:IR",
	 "LINE1:Thr#00FF00:Thr",
	 "LINE1:Tp#FF0000:Tp",
	 "LINE1:Tir#0000FF:Tir",
	 "HRULE:0#00FFFFAA:ZERO",
	 "AREA:Dew#00008F10:Dew\\r")


	ret = rrdtool.graph( CHARTPATH+"pressure"+str(time)+".png","-A","--start","-"+str(time)+"h","-E",
          preamble,
	 "--title","Pressure",
	 "--vertical-label=mBars",
	 "-u",str(Pmax),
	 "-l",str(Pmin),
	 "-r",
	 "DEF:P=meteo.rrd:P:AVERAGE",
	 "HRULE:"+str(P0)+"#"+red+"AA:standard",
	 "LINE1:P#"+blue+":P\\r",
	 "COMMENT:\\n",
	 "GPRINT:P:AVERAGE:Avg P\: %6.2lf %S\\r")

	ret = rrdtool.graph( CHARTPATH+"hr"+str(time)+".png","--start","-"+str(time)+"h","-E",
          preamble,
	 "-u","105",
	 "-l","-5",
	 "-r",
	 "--title","Humidity",
	 "--vertical-label=%",
	 "DEF:HR=meteo.rrd:HR:AVERAGE",
	 "HRULE:100#FF00FFAA:100%",
	 "HRULE:0#00FFFFAA:0%",
	 "LINE1:HR#"+blue+":HR\\r",
	 "COMMENT:\\n",
	 "GPRINT:HR:AVERAGE:Avg HR\: %6.2lf %S\\r")

	ret = rrdtool.graph( CHARTPATH+"light"+str(time)+".png","--start","-"+str(time)+"h","-E",
          preamble,
	 "--title","Iradiance",
	 "--vertical-label=rel",
	 "DEF:Light=meteo.rrd:Light:AVERAGE",
	 "LINE1:Light#"+blue+":Irradiance\\r",
	 "COMMENT:\\n",
	 "GPRINT:Light:AVERAGE:Avg Light\: %6.2lf %S\\r")

	ret = rrdtool.graph( CHARTPATH+"clouds"+str(time)+".png","-A","--start","-"+str(time)+"h","-E",
          preamble,
	 "--title","Clouds",
	 "--vertical-label=%",
	 "-u","102",
	 "-l","-2",
	 "-r",
	 "DEF:clouds=meteo.rrd:clouds:AVERAGE",
	 "DEF:cloudFlag=meteo.rrd:cloudFlag:AVERAGE",
	 "CDEF:cloudy=clouds,cloudFlag,*",
	 "LINE1:clouds#"+orange+":clouds",
	 "AREA:cloudy#FFFFFF40:CloudyFlag\\r",
	 "AREA:30#00000a40:Clear",
	 "AREA:40#0000AA40:Cloudy:STACK",
	 "AREA:32#0000FF40:Overcast:STACK")

	ret = rrdtool.graph( CHARTPATH+"skyT"+str(time)+".png","--start","-"+str(time)+"h","-E",
          preamble,
	 "--title","Sky Temperatures",
	 "--vertical-label=Celsius ºC",
	 "DEF:skyT=meteo.rrd:skyT:AVERAGE",
	 "DEF:IR=meteo.rrd:IR:AVERAGE",
	 "DEF:Thr=meteo.rrd:Thr:AVERAGE",
	 "CDEF:Tc=IR,skyT,-",
	 "LINE1:skyT#"+blue+":Corrected Sky T",
	 "LINE1:IR#"+orange+":Actual Sky T",
	 "LINE1:Thr#"+red+":Ambient T",
	 "LINE1:Tc#"+white+":Correction",
	 "HRULE:0#00FFFFAA:ZERO",
	 "COMMENT:\\n",
	 "GPRINT:skyT:AVERAGE:Avg Sky Temp\: %6.2lf %S\\r")

P0=math.floor(1013.25/math.exp(ALTITUDE/8000.))
Pdelta=25
Pmin=P0-Pdelta
Pmax=P0+Pdelta
i=0
print "Starting GRAPHER"
while (True):
	graphs(3)
	print "Generating 3 hours graph"
	if (i % 4 == 0):
		print "Generating day graph"
		graphs(24)
	if (i % 21 == 0):
		print "Generating weekly graph"
		graphs(168)
	if (i % 147 == 0):
		print "Generating monthly graph"
		graphs(1176)
        i=i+1
	time.sleep(60)
