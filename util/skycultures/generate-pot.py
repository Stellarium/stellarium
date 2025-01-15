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

# Script that generates stellarium-skyculture-descriptions.pot

import os
import re
import sys
import polib

if sys.version_info[0] < 3:
    raise Exception("Please use Python 3 (current is {})".format(
        sys.version_info[0]))

DIR = os.path.abspath(os.path.dirname(__file__))
SCDIR = os.path.join(DIR, '..', '..', 'skycultures')
POTFILE = os.path.join(DIR, '..', '..', 'po', 'stellarium-skycultures-descriptions', 'stellarium-skycultures-descriptions.pot')

LANGS = ['af','am','ar','arn','ay','az','be','bg','bn','br','bs','ca',
         'ca@valencia','cs','da','de','el','en','en_AU','en_CA','en_GB','en_US',
         'es','es_419','et','eu','fa','fi','fr','fy','gd','gl','gu','he','hi',
         'hr','hu','hy','id','is','it','ja','jv','ka','ko','la','lb','lt','lv',
         'mi','ml','mr','ms','nb','nds','ne','nl','nn','oj','pl','pt','pt_BR',
         'qu','rap','ro','ru','sa','sc','shi','si','sk','sl','sq','sr','sr@latin',
         'sv','ta','te','th','ti','tr','tt','uk','vi','zh_CN','zh_HK','zh_TW']

def is_sky_culture_dir(d):
    if not os.path.isdir(os.path.join(SCDIR, d)):
        return False
    index_file = os.path.join(SCDIR, d, 'index.json')
    return os.path.exists(index_file)

def main():
    sclist = [d for d in os.listdir(SCDIR) if is_sky_culture_dir(d)]
    sclist.sort()

    pot = polib.POFile(encoding='utf-8', check_for_duplicates=True)
    pot.metadata = {
        'Project-Id-Version': 'PACKAGE VERSION',
        'Report-Msgid-Bugs-To': 'stellarium@googlegroups.com',
        'Last-Translator': 'FULL NAME <EMAIL@ADDRESS>',
        'Language-Team': 'LANGUAGE <LL@li.org>',
        'Language': '',
        'MIME-Version': '1.0',
        'Content-Type': 'text/plain; charset=UTF-8',
        'Content-Transfer-Encoding': '8bit'
    }

    for sky_culture in sclist:
        data_path = os.path.join(SCDIR, sky_culture)
        description_file = os.path.join(data_path, 'description.md')
        assert os.path.exists(description_file)

        with open(description_file) as file:
            markdown = file.read()
            # First strip all the comments
            markdown = re.sub(r'<!--.*?-->', "", markdown)

            titleRE = r'(?:^|\n)\s*# ([^\n]+)\n'
            titleL1 = re.findall(titleRE, markdown)
            if len(titleL1) != 1:
                print(f"{sky_culture}: There must be one level-1 title, instead found {len(titleL1)}: {titleL1}", file=sys.stderr)
                sys.exit(1)

            sc_name = titleL1[0]

            # First make cases with empty sections following each other with only a single line
            # in between separated by _two_ lines, so that the split below works in this case too.
            markdown = re.sub(r'\n[ \t]*##([^\n#][^\n]*)\n##([^#\n])', r'\n##\1\n\n##\2', markdown)

            sectionsL2 = re.split(r'\n[ \t]*##([^\n#][^\n]*)\n', markdown)
            if len(sectionsL2) < 3:
                print(f"{sky_culture}: Bad number of level-2 sections in description", file=sys.stderr)
                sys.exit(1)

            assert re.search(titleRE, sectionsL2[0])[0] == sectionsL2[0]
            assert len(sectionsL2) % 2 == 1 # The list is: [scName, sectionTitle, sectionBody, ..., sectionTitle, sectionBody]

            entry = polib.POEntry(tcomment = sky_culture + " sky culture name", msgid = sc_name, msgstr = "")
            pot.append(entry)

            standard_l2_titles = ['Introduction', 'Description', 'Constellations', 'References', 'Authors', 'License']
            for n in range(1,len(sectionsL2),2):
                sec_title = sectionsL2[n].strip()
                sec_body = re.sub(r'^\n+', "", sectionsL2[n+1].rstrip())

                if re.search("^[ \t]*##[ \t]*Constellations[ \t]*\n", sec_body):
                    print(f"Bad section body computed:\n{sec_body}", file=sys.stderr)
                    sys.exit(1)

                if sec_title not in standard_l2_titles:
                    print(f"{sky_culture}: Invalid level-2 section title: {sec_title}. Must be one of {standard_l2_titles}.", file=sys.stderr)
                    sys.exit(1)

                if len(sec_body) == 0:
                    print(f'{sky_culture}: warning: empty section "{sec_title}"', file=sys.stderr)
                    continue

                entry = polib.POEntry(tcomment = sky_culture + ' sky culture ' + sec_title.lower() + ' section in markdown format',
                                      msgid = sec_body, msgstr = "")
                if entry not in pot: # Can happen e.g. for licenses
                    pot.append(entry)

    pot.save(POTFILE)

if __name__ == '__main__':
    main()
