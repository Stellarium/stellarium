#!/bin/bash

# Tool for extract of the translatable lines from XML file and generation of the localized metainfo file.
# This is modified KDE script (Original source: http://websvn.kde.org/trunk/l10n-kde4/scripts/extract_metainfo.sh)
# Required itstool >=1.2.0

ROOT=../..
ASMETAINFOITS=./as-metainfo.its
podir=${ROOT}/po/stellarium-metainfo
metainfo_file=${ROOT}/data/stellarium.appdata.xml

# first, strip translation from project metadata file
tmpxmlfile=$(mktemp)
xmlstarlet ed -d "//*[@xml:lang]" $metainfo_file > $tmpxmlfile

# get rid of the .xml extension and dirname
metainfo_file_basename=$(basename $metainfo_file)
dataname="stellarium-metainfo"

# create pot file stellarium.appdata.xml -> stellarium.appdata.pot
itstool -i $ASMETAINFOITS -o $podir/$dataname.pot $tmpxmlfile
esc_tmpxmlfile=$(echo $tmpxmlfile|sed -e 's/[]\/()$*.^|[]/\\&/g')
sed -i "/^#:/s/$esc_tmpxmlfile/$metainfo_file_basename/" $podir/$dataname.pot

tmpmofiles=""
tmpdir=$(mktemp -d)
# find po files in l10n module dir
for pofile in $(ls $podir/*.po 2> /dev/null); do
    # get language-id
    lang=$(basename $pofile)
    lang=$(echo $lang|sed 's/.po//')

    # generate mo files (need to be named after their language)
    mofile="$tmpdir/$lang.mo"
    msgfmt $pofile -o $mofile

    tmpmofiles="$tmpmofiles $mofile"
done

if [ -n "$tmpmofiles" ]; then
    # recreate file, using the untranslated temporary data and the translation
    itstool -i $ASMETAINFOITS -j $tmpxmlfile -o $metainfo_file $tmpmofiles
fi

# cleanup
rm -rf $tmpdir
rm $tmpxmlfile

# update list of releases
./update_releases_appdata.pl

# format (pretty print)
xmllint --format --output $metainfo_file $metainfo_file

