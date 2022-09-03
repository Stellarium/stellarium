#!/bin/bash

po4a --verbose --force --msgid-bugs-address stellarium@googlegroups.com --copyright-holder "Stellarium's team" ./po4a.config

#for i in ../../po/stellarium-landscapes-descriptions/*.po
#do
#	msgmerge --previous -o $i $i ../../po/stellarium-landscapes-descriptions/stellarium-landscapes-descriptions.pot
#done
