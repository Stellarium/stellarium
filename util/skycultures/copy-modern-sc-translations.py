#!/usr/bin/python3

import re
import sys
import polib
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("filename", help="Translations file to process")
args = parser.parse_args()

pofile = polib.pofile(args.filename)

nativeToMsgId = {}
msgidToTr = {}
cons_re = re.compile(r'Modern constellation, native: ([^\n,]+)')
# First look for all Modern constellations that want
# a translation, registering their native names.
for entry in pofile:
    if entry.msgctxt != 'IAU constellation name':
        continue
    if entry.msgstr:
        continue
    if not entry.comment:
        continue
    match = cons_re.match(entry.comment)
    if not match:
        continue
    native = match.group(1)
    msgid = entry.msgid
    if msgid == 'Boötes':
        # Old translations have this version in msgid
        msgid = 'Bootes'
        native = 'Bootes'
    print(f'Registering {native} -> {msgid}', file=sys.stderr)
    nativeToMsgId[native] = msgid

# Now for all native names registered find a translation
for entry in pofile:
    msgid = nativeToMsgId.get(entry.msgid)
    if not msgid:
        continue
    if not entry.msgstr:
        print(f'WARNING: no translation for {msgid}', file=sys.stderr)
    else:
        print(f'Found {msgid} -> {entry.msgstr}', file=sys.stderr)
        msgidToTr[msgid] = entry.msgstr

# Finally, for all Modern constellation names add the corresponding translation
for entry in pofile:
    if entry.msgctxt != 'IAU constellation name':
        continue
    if entry.msgstr:
        print(f'Translation for {entry.msgid} already exists: {entry.msgstr}, skipping it', file=sys.stderr)
        continue
    msgid = entry.msgid
    if msgid == 'Boötes':
        # Old translations have this version in msgid
        msgid = 'Bootes'
    tr = msgidToTr.get(msgid)
    if not tr:
        continue
    print(f'Translating {entry.msgid} -> {tr}', file=sys.stderr)
    entry.msgstr = tr
    pofile.save(args.filename)
