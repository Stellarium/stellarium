#!/bin/bash

rm -f new.json

GET "http://celestrak.com/NORAD/elements/goes.txt"    | ./add_new_from_tle_file.pl scientific >> new.json
GET "http://celestrak.com/NORAD/elements/galileo.txt" | ./add_new_from_tle_file.pl experimental navigation >> new.json
GET "http://celestrak.com/NORAD/elements/noaa.txt"    | ./add_new_from_tle_file.pl scientific weather non-operational >> new.json
GET "http://celestrak.com/NORAD/elements/gps-ops.txt" | ./add_new_from_tle_file.pl gps navigation >> new.json
GET "http://celestrak.com/NORAD/elements/amateur.txt" | ./add_new_from_tle_file.pl amateur >> new.json
GET "http://celestrak.com/NORAD/elements/iridium.txt" | ./add_new_from_tle_file.pl iridium communications >> new.json
GET "http://celestrak.com/NORAD/elements/visual.txt"  | ./add_new_from_tle_file.pl visual >> new.json
GET "http://celestrak.com/NORAD/elements/geo.txt"     | ./add_new_from_tle_file.pl geostationary >> new.json

echo "now splice new.json into the old satellites.json file"
