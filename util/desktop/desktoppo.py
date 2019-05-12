#!/usr/bin/env python
# -*- coding: utf-8 -*-

import argparse
import re
import sys
from pathlib import Path
from time import strftime

import polib


def write_pot_file(desktop_file: Path, po_dir: Path):
    pot = polib.POFile('', check_for_duplicates=True)
    potcreationtime = strftime('%Y-%m-%d %H:%M%z')
    pot.metadata = {
        'Project-Id-Version': 'Stellarium desktop file translation',
        'Report-Msgid-Bugs-To': 'stellarium@googlegroups.com',
        'POT-Creation-Date': potcreationtime,
        'PO-Revision-Date': 'YEAR-MO-DA HO:MI+ZONE',
        'Last-Translator': 'FULL NAME <EMAIL@ADDRESS>',
        'Language-Team': 'LANGUAGE <LL@li.org>',
        'MIME-Version': '1.0',
        'Content-Type': 'text/plain; charset=UTF-8',
        'Content-Transfer-Encoding': '8bit',
    }

    # open desktop file
    with open(desktop_file, "r") as f:
        text = f.read()

    # Parse contents and add them to POT
    for mblock in mpattern.findall(text):
        mblock_stripped = mblock.strip('\n')
        message_comment, message_id = mblock.strip('\n').split('=')
        potentry = polib.POEntry(
            msgctxt=message_comment,
            msgid=message_id.decode('utf-8'),
            msgstr='',
            occurrences=[(langfile, '')]
        )
        if message_id != '':
            try:
                pot.append(potentry)
            except ValueError:
                print('The entry already exists')
    pot.save(po_dir.joinpath('stellarium-desktop.pot'))


def main():
    parser = argparse.ArgumentParser(
        description='generate pot catalogs and updates po files from a desktop resources')
    parser.add_argument(dest='input', type=Path, metavar='file', help='desktop file')
    parser.add_argument(dest='output_dir', type=Path, default='po/stellarium-desktop/', metavar='directory',
                        help='output directory')

    args = parser.parse_args()

    desktop_file = Path(args.input)
    po_dir = Path(args.output_dir)
    if not desktop_file.exists():
        print(f"'{desktop_file}' do not exist.")
        sys.exit(1)
    if not po_dir.exists():
        print(f"'{po_dir}' do not exist and will be created.")
        po_dir.mkdir(parents=True, exist_ok=True)

    # Define Templates and po directory name
    messagetemplate = '(?<=\n)Name=.*?\n|GenericName=.*?\n|Comment=.*?\n'
    mpattern = re.compile(messagetemplate, re.DOTALL)
    translationtemplate = '(?<=\n)Name\[.*?\n|GenericName\[.*?\n|Comment\[.*?\n'
    tpattern = re.compile(translationtemplate, re.DOTALL)

    # Write POT file
    write_pot_file()
    return

    # Merge translations
    for pofile in glob.glob(podir + '/*.po'):
        lang = pofile[:-3].rsplit('/', 1)[1]
        pofilename = pofile
        po = polib.pofile(pofilename)
        po.merge(pot)
        po.save(pofilename)

    with open(desktop_file, "r") as deskfile:
        text = deskfile.read()
    with open(desktop_file, "w") as deskfile:
        for transblock in tpattern.findall(text):
            text = text.replace(transblock, '')

        # Parse PO files
        for pofile in sorted(glob.glob(podir + '/*.po'), reverse=True):
            lang = pofile[:-3].rsplit('/', 1)[1]
            pofilename = pofile
            po = polib.pofile(pofilename)
            for entry in po.translated_entries():
                if entry.msgid.encode('utf-8') in text:
                    origmessage = ('\n' + entry.msgctxt + '=' + entry.msgid + '\n').encode('utf-8')
                    origandtranslated = (
                            '\n' + entry.msgctxt + '=' + entry.msgid + '\n' + entry.msgctxt + '[' + lang + ']=' + entry.msgstr + '\n').encode(
                        'utf-8')
                    text = text.replace(origmessage, origandtranslated)

        deskfile.write(text)


if __name__ == '__main__':
    main()
