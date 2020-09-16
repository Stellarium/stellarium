#!/bin/bash

po4a po4a.config

for i in ../../po/stellarium-landscapes-descriptions/*.po
do
	msgmerge --previous -o $i $i ../../po/stellarium-landscapes-descriptions/stellarium-landscapes-descriptions.pot
done
