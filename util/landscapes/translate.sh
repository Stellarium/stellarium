#!/bin/bash

LANG=$1

if [[ $LANG == "" ]]; then
    for PO in ../../po/stellarium-landscapes-descriptions/*.po
    do
        pofilename=$(basename -- $PO)
        for i in ../../landscapes/*/description.en.utf8
        do
            po4a-translate -k 0 -f xhtml -m $i -M utf-8 -p $PO -l ${i%.*.*}.${pofilename%.*}.utf8
        done
    done
else
    for i in ../../landscapes/*/description.en.utf8
    do
        po4a-translate -k 0 -f xhtml -m $i -M utf-8 -p ../../po/stellarium-landscapes-descriptions/$LANG.po -l ${i%.*.*}.$LANG.utf8
    done
fi
