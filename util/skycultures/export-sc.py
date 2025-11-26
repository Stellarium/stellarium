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

# This script generates .po and .pot files for a given sky culture, so that
# it can be put into the stellarium-skycultures repo and imported from there.

import os
import sys
import argparse

if sys.version_info[0] < 3:
    raise Exception("Please use Python 3 (current is {})".format(
        sys.version_info[0]))

DIR = os.path.abspath(os.path.dirname(__file__))
SCDIR = os.path.join(DIR, '..', '..', 'skycultures')
DESCPOTFILE = os.path.join(DIR, '..', '..', 'po', 'stellarium-skycultures-descriptions', 'stellarium-skycultures-descriptions.pot')
SCPOTFILE = os.path.join(DIR, '..', '..', 'po', 'stellarium-skycultures', 'stellarium-skycultures.pot')
OUT_DIR_I18N_SC = os.path.join(DIR, '..', '..', 'po', 'stellarium-skycultures')
OUT_DIR_I18N_GUI = os.path.join(DIR, '..', '..', 'po', 'stellarium-skycultures-descriptions')

parser = argparse.ArgumentParser()
parser.add_argument('-s', '--sky-culture', help='Export the sky culture specified')
parser.add_argument("--skip-abbrev", action="store_true", help="Don't emit translations for abbreviations")
parser.add_argument("--skip-zodiac", action="store_true", help="Don't emit translations for Zodiac or Lunar System entries")
parser.add_argument("--skip-pronounce", action="store_true", help="Don't emit translations for 'pronounce' entries")
args = parser.parse_args()

if not args.sky_culture:
    print('No sky culture specified', file=sys.stderr)
    sys.exit(1)

sky_culture = args.sky_culture

langs=[]
for filename in os.listdir(OUT_DIR_I18N_SC) + os.listdir(OUT_DIR_I18N_GUI):
    if filename.endswith('.po'):
        lang = filename.replace('.po', '')
        if not os.path.exists(os.path.join(OUT_DIR_I18N_SC, lang + '.po')):
            print(f"Warning: no SC object name file for language {lang}", file=sys.stderr)
            continue
        if not os.path.exists(os.path.join(OUT_DIR_I18N_GUI, lang + '.po')):
            print(f"Warning: no SC description file for language {lang}", file=sys.stderr)
            continue
        langs.append(lang)
langs = list(set(langs))
langs.sort()

if os.system(os.path.join(DIR, 'generate-pot.py') + f" -s {sky_culture}" +
             (" --skip-abbrev" if args.skip_abbrev else "") +
             (" --skip-zodiac" if args.skip_zodiac else "") +
             (" --skip-pronounce" if args.skip_pronounce else "")) != 0:
    sys.exit(1)
# Output file of the pot generator
pot_path = os.path.join(SCDIR, sky_culture, 'po', sky_culture + '.pot')

for lang in langs:
    print(f"Generating SC-specific po file {lang}.po...", file=sys.stderr)

    sc_po_path = os.path.join(OUT_DIR_I18N_SC, lang + '.po')
    gui_po_path = os.path.join(OUT_DIR_I18N_GUI, lang + '.po')
    tmp_po_path = os.path.join(os.path.dirname(pot_path), lang + '.po.tmp')
    po_path = os.path.join(os.path.dirname(pot_path), lang + '.po')
    po_files= [sc_po_path, gui_po_path]

    if os.system('msgcat --use-first --force-po "'+('" "'.join(po_files))+f'" -o "{tmp_po_path}"') != 0:
        sys.exit(1)

    if os.system(f'msgmerge -N -o "{po_path}" "{tmp_po_path}" "{pot_path}"') != 0:
        sys.exit(1)
    os.remove(tmp_po_path)

    # Remove the obsolete entries marked with #~
    if os.system(f'msgattrib --no-obsolete -o "{po_path}" "{po_path}"') != 0:
        sys.exit(1)
