#!/bin/bash

po4a -M UTF-8 -L UTF-8 -o untranslated="<notr>" --verbose --force --msgid-bugs-address stellarium@googlegroups.com --copyright-holder "Stellarium's team" ./po4a.config

#for i in ../../po/stellarium-skycultures-descriptions/*.po
#do
#	msgmerge --previous -o $i $i ../../po/stellarium-skycultures-descriptions/stellarium-skycultures-descriptions.pot
#done
