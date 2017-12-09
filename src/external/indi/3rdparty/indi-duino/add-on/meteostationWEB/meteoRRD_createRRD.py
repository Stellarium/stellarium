#!/usr/bin/python
#-*- coding: iso-8859-15 -*-
# INDUINO METEOSTATION
# http://induino.wordpress.com 
# 
# NACHO MAS 2013

import sys
import rrdtool

#10s raw values for 3hour, 1min for 24 hours, 5 min for 24*7 hours,
# 1hour for 1 year, 1day dor 10 years!
ret = rrdtool.create("meteo.rrd", "--step", "1", "--start", '0',
		 "DS:HR:GAUGE:600:U:U",
		 "DS:Thr:GAUGE:600:U:U",
		 "DS:IR:GAUGE:600:U:U",
		 "DS:Tir:GAUGE:600:U:U",
		 "DS:P:GAUGE:600:U:U",
		 "DS:Tp:GAUGE:600:U:U",
		 "DS:Dew:GAUGE:600:U:U",
		 "DS:Light:GAUGE:600:U:U",
		 "DS:T:GAUGE:600:U:U",
		 "DS:clouds:GAUGE:600:U:U",
		 "DS:skyT:GAUGE:600:U:U",
		 "DS:cloudFlag:GAUGE:600:U:U",
		 "DS:dewFlag:GAUGE:600:U:U",
		 "DS:frezzingFlag:GAUGE:600:U:U",
		 "RRA:AVERAGE:0.5:1:10800",
		 "RRA:AVERAGE:0.5:60:1440",
		 "RRA:AVERAGE:0.5:300:1008",
		 "RRA:AVERAGE:0.5:3600:8760",
		 "RRA:AVERAGE:0.5:86400:3650",
		 "RRA:MAX:0.5:1:10800",
		 "RRA:MAX:0.5:60:1440",
		 "RRA:MAX:0.5:300:1008",
		 "RRA:MAX:0.5:3600:8760",
		 "RRA:MAX:0.5:86400:3650",
		 "RRA:MIN:0.5:1:10800",
		 "RRA:MIN:0.5:60:1440",
		 "RRA:MIN:0.5:300:1008",
		 "RRA:MIN:0.5:3600:8760",
		 "RRA:MIN:0.5:86400:3650")


if ret:
		print rrdtool.error()

