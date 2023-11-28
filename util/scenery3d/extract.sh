#!/bin/bash

po4a -M UTF-8 -L UTF-8 --verbose --force --msgid-bugs-address stellarium@googlegroups.com --copyright-holder "Stellarium's team" ./po4a.config

#for i in ../../po/stellarium-scenery3d-descriptions/*.po
#do
#	msgmerge --previous -o $i $i ../../po/stellarium-scenery3d-descriptions/stellarium-scenery3d-descriptions.pot
#done
