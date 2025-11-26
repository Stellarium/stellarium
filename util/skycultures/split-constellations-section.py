#!/usr/bin/python3

# Stellarium
# Copyright (C) 2025 - Stellarium Labs SRL
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.

# Script that fixes the Constellation section being
# presented monolithically in the .po files.

import os
import re
import sys
import polib
import argparse

if sys.version_info[0] < 3:
    raise Exception("Please use Python 3 (current is {})".format(
        sys.version_info[0]))

DIR = os.path.abspath(os.path.dirname(__file__))
SCDIR = os.path.join(DIR, '..', '..', 'skycultures')

parser = argparse.ArgumentParser()
parser.add_argument("filename", help="Translations file to process")
args = parser.parse_args()

def handle_constellations_section(sky_culture, sc_name, body, names):
    body = body.strip()
    if len(re.findall('^#####[^#]', body)) != 1:
        print(f'{sky_culture}: warning: badly formatted Constellations section', file=sys.stderr)
        return False

    subsections = re.split(r'(?:\n|^)[ \t]*#####[ \t]*([^#\n][^\n]*)\n', body)
    if subsections[0] != '':
        printf(f'{sky_culture}: warning: unexpected beginning of Constellations section, splitting failed', file=sys.stderr)
        return False
    subsections = subsections[1:]
    if len(subsections) % 2 != 0:
        print(f'{sky_culture}: warning: odd number ({len(subsections)}) of Constellations section title/body items, splitting failed', file=sys.stderr)
        return False

    constellations_added = 0
    for s in range(int(len(subsections) / 2)):
        title = subsections[s * 2]
        descr = subsections[s * 2 + 1].strip()
        comment = f'Description of {sc_name} constellation {title}'
        names.append({'msg': descr, 'comment': comment})
        constellations_added += 1
    return constellations_added > 0


pofile = polib.pofile(args.filename)
section_re = re.compile(r'(.*) sky culture constellations section')
replacedAnyConstellations = False
for entry in pofile:
    if not entry.comment:
        continue
    if not entry.msgstr:
        continue
    match = section_re.match(entry.comment)
    if not match:
        continue

    sc_name = match.group(1)
    eng_body = entry.msgid
    tr_body = entry.msgstr
    eng_names = []
    if not handle_constellations_section(sc_name, sc_name, eng_body, eng_names):
        continue
    tr_names = []
    if not handle_constellations_section(sc_name, sc_name, tr_body, tr_names):
        continue

    if len(eng_names) != len(tr_names):
        print(f'Numbers of English names and translated names differ: '
              f'{len(eng_names)} vs {len(tr_names)}', file=sys.stderr)
        continue

    print(f'Successfully found constellations in {sc_name}', file=sys.stderr)

    pofile.remove(entry)

    for n in range(len(eng_names)):
        comment = eng_names[n]['comment']
        entry = polib.POEntry(comment = comment,
                              msgid = eng_names[n]['msg'],
                              msgstr = tr_names[n]['msg'])
        if entry in pofile:
            prev_entry = pofile.find(entry.msgid)
            assert prev_entry
            prev_entry.comment += '\n' + comment
        else:
            pofile.append(entry)

    replacedAnyConstellations = True

if replacedAnyConstellations:
    pofile.save(args.filename)
