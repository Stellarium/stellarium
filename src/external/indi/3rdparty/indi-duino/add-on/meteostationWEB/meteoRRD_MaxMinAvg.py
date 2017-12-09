#!/usr/bin/python
#-*- coding: iso-8859-15 -*-
# INDUINO METEOSTATION
# http://induino.wordpress.com
#
# NACHO MAS 2013

import sys, os
import math
import time
import rrdtool
from meteoconfig import *
import simplejson

def writeJson(consolidation,resolution):
   now=time.time()
   json_dict={}
   res=resolution
   end=int((now)/res)*res
   start=end-res
   #print now,start,end
   filename=CHARTPATH+consolidation+"values.json"
   try:
	ret = rrdtool.fetch('meteo.rrd',consolidation,"--start",str(start),"--end",str(end),"--resolution",str(res));
	if ret:
	 	print rrdtool.error()
	#print ret
	mags=ret[1]
	values=ret[2][0]
	i=0
	for mag in mags:
		#print mag,values[i]
		json_dict[mag]=int(values[i]*100)/100.
		i=i+1
	#print consolidation,json_dict
	x = simplejson.dumps(json_dict)
        fi=open(filename,"w")
	fi.write(x)
	fi.close()
   except:
	os.remove(filename)

############# MAIN #############
print "Starting MinMax"
while (True):
  try:
	writeJson("AVERAGE",3600)
	writeJson("MAX",86400)
	writeJson("MIN",86400)
	time.sleep(10)
  except:
	print "MinMax FAIL"
	time.sleep(10)
