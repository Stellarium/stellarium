#!/bin/bash

for i in */*en.utf8
do
	po4a-translate -k 0 -f xhtml -m $i -M utf-8 -p po/uk.po -l ${i%.*.*}.uk.utf8
done
