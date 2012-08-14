#!/bin/bash

# This script downloads lists of satellite TLE sets from celestrak.com
# and generates a list of those that don't exist in the satellites.json file.
# Note that the "add_new_from_tle_file.pl" script assumes that satellites.json
# is in the current working directory.
# The output (new.json) needs to be appended manually to satellites.json
# The other part of keeping satellites.json up to date - removing satellites
# that no longer exist in the lists - you will have to do manually, too.

rm -f new.json

GET "http://celestrak.com/NORAD/elements/goes.txt"    | ./add_new_from_tle_file.pl scientific >> new.json
GET "http://celestrak.com/NORAD/elements/galileo.txt" | ./add_new_from_tle_file.pl experimental navigation >> new.json
GET "http://celestrak.com/NORAD/elements/noaa.txt"    | ./add_new_from_tle_file.pl scientific weather non-operational >> new.json
GET "http://celestrak.com/NORAD/elements/gps-ops.txt" | ./add_new_from_tle_file.pl gps navigation >> new.json
GET "http://celestrak.com/NORAD/elements/amateur.txt" | ./add_new_from_tle_file.pl amateur >> new.json
GET "http://celestrak.com/NORAD/elements/iridium.txt" | ./add_new_from_tle_file.pl iridium communications >> new.json
GET "http://celestrak.com/NORAD/elements/visual.txt"  | ./add_new_from_tle_file.pl visual >> new.json
GET "http://celestrak.com/NORAD/elements/geo.txt"     | ./add_new_from_tle_file.pl geostationary >> new.json

echo "Now splice new.json into the old satellites.json file!"
