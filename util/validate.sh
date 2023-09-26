#!/bin/sh

echo "Check appdata.xml file:"
appstream-util validate-relax --nonet ../data/org.stellarium.Stellarium.appdata.xml

echo "\nCheck stellarium.desktop file:"
desktop-file-validate ../data/org.stellarium.Stellarium.desktop