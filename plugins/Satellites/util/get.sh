#!/bin/bash

GET http://celestrak.com/NORAD/elements/noaa.txt > new.tle
GET http://celestrak.com/NORAD/elements/goes.txt >> new.tle
GET http://celestrak.com/NORAD/elements/gps-ops.txt >> new.tle
GET http://celestrak.com/NORAD/elements/galileo.txt >> new.tle
GET http://celestrak.com/NORAD/elements/visual.txt >> new.tle
GET http://celestrak.com/NORAD/elements/amateur.txt >> new.tle
GET http://celestrak.com/NORAD/elements/iridium.txt >> new.tle
GET http://celestrak.com/NORAD/elements/tle-new.txt >> new.tle
GET http://celestrak.com/NORAD/elements/geo.txt >> new.tle


