#!/bin/bash

po4a po4a.config

for i in ../../po/stellarium-skyculture-descriptions/*.po
do
	msgmerge --previous -o $i $i ../../po/stellarium-skyculture-descriptions/stellarium-skyculture-descriptions.pot
done
