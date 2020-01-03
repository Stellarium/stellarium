#!/bin/bash

po4a po4a.config

for i in po/*.po
do
	msgmerge --previous -o $i $i po/skycultures.pot
done
