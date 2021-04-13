#!/bin/bash

po4a po4a.config

for i in ../../po/stellarium-scenery3d-descriptions/*.po
do
	msgmerge --previous -o $i $i ../../po/stellarium-scenery3d-descriptions/stellarium-scenery3d-descriptions.pot
done
