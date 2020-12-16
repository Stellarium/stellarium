#!/bin/bash

po4a po4a.config

for i in ../../po/stellarium-skycultures-descriptions/*.po
do
	msgmerge --previous -o $i $i ../../po/stellarium-skycultures-descriptions/stellarium-skycultures-descriptions.pot
done
