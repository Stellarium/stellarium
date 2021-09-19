#!/bin/bash

po4a --verbose --force --msgid-bugs-address stellarium@googlegroups.com --copyright-holder "Stellarium's team" ./po4a.config

#for i in ../../po/stellarium-skycultures-descriptions/*.po
#do
#	msgmerge --previous -o $i $i ../../po/stellarium-skycultures-descriptions/stellarium-skycultures-descriptions.pot
#done
