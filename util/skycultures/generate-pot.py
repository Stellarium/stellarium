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

# Script that generates two .pot files:
#  * stellarium-skycultures.pot
#  * stellarium-skycultures-descriptions.pot

import os
import re
import sys
import json
import polib
import argparse

if sys.version_info[0] < 3:
    raise Exception("Please use Python 3 (current is {})".format(
        sys.version_info[0]))

DIR = os.path.abspath(os.path.dirname(__file__))
SCDIR = os.path.join(DIR, '..', '..', 'skycultures')
DESCPOTFILE = os.path.join(DIR, '..', '..', 'po', 'stellarium-skycultures-descriptions', 'stellarium-skycultures-descriptions.pot')
SCPOTFILE = os.path.join(DIR, '..', '..', 'po', 'stellarium-skycultures', 'stellarium-skycultures.pot')

sc_names = {}
common_names = set()
cons_ast_names = set()
cons_names_for_describing = dict()

def ensure_dir(file_path):
    '''Create a directory for a path if it doesn't exist yet'''
    directory = os.path.dirname(file_path)
    if not os.path.exists(directory):
        os.makedirs(directory)

def is_sky_culture_dir(d):
    if not os.path.isdir(os.path.join(SCDIR, d)):
        return False
    index_file = os.path.join(SCDIR, d, 'index.json')
    return os.path.exists(index_file)

def load_common_names():
    star_names_path = os.path.join(SCDIR, 'common_star_names.fab')
    dso_names_path = os.path.join(DIR, '..', '..', 'nebulae', 'default', 'names.dat')
    comment_re = re.compile(r'^#|^\s*$')
    entry_re = re.compile(r'\s*[^|]\+\s*|\s*_\("([^"]+)"\)')
    for path in [star_names_path, dso_names_path]:
        with open(path) as file:
            line_num = 0
            for line in file:
                line_num += 1
                if comment_re.search(line):
                    continue
                match = entry_re.search(line)
                if not match:
                    print(f"{path}:{line_num}: error: failed to parse line", file=sys.stderr)
                    continue
                common_names.add(match.group(1))
    print(f"Loaded {len(common_names)} common names", file=sys.stderr)

def handle_constellations_section(sky_culture, sc_name, body, pot):
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
        if title not in cons_names_for_describing[sky_culture]:
            print(f'{sky_culture}: warning: no constellation named "{title}" in index', file=sys.stderr)
            continue

        comment = f'Description of {sc_name} constellation {title}'
        entry = polib.POEntry(comment = comment, msgid = descr, msgstr = "")
        if entry in pot:
            prev_entry = pot.find(entry.msgid)
            assert prev_entry
            prev_entry.comment += '\n' + comment
        else:
            pot.append(entry)

        constellations_added += 1

    return constellations_added > 0

def checkReferences(sky_culture, sec_body):
    ref_re = re.compile(r'^\s*- \[#([0-9]+)\]: \s*\S+')
    refNum = 1
    for line in sec_body.splitlines():
        if line.strip() == '':
            continue
        match = ref_re.search(line)
        if not match:
            print(f"{sky_culture}: warning: invalid reference line:\n{line}", file=sys.stderr)
            continue
        newRefNum = match.group(1)
        if int(newRefNum) != refNum:
            print(f"{sky_culture}: warning: reference number isn't equal to previous+1:\n{line}", file=sys.stderr)
        refNum += 1

def checkDescriptionSectionHeaders(sky_culture, text):
    # Reference: https://www.markdownguide.org/basic-syntax/#heading-best-practices

    extraSpaceBeforeHashRE = re.compile(r'(?:^|\n)[ 	]+#{1,6}[^\n]*')
    noSpaceAfterHashRE = re.compile(r'(?:^|\n)[ 	]*#{1,6}[^# ][^\n]*')
    for match in extraSpaceBeforeHashRE.finditer(text):
        print((f'{sky_culture}: warning: extra space before hash in section header (see '
               f'https://www.markdownguide.org/basic-syntax/#heading-best-practices):\n'
               f'{match.group(0)}'), file=sys.stderr)
    for match in noSpaceAfterHashRE.finditer(text):
        print((f'{sky_culture}: warning: no space after hash in section header (see '
               f'https://www.markdownguide.org/basic-syntax/#heading-best-practices):\n'
               f'{match.group(0)}'), file=sys.stderr)

    noBlankLineBeforeHeader = re.compile(r'[^\n]\n(##[^\n]*)')
    noBlankLineAfterHeader = re.compile(r'(?:^|\n)(##[^\n]*)\n[^\n]')
    for match in noBlankLineBeforeHeader.finditer(text):
        print((f'{sky_culture}: warning: no empty line before section header (see '
               f'https://www.markdownguide.org/basic-syntax/#heading-best-practices):\n'
               f'{match.group(1)}'), file=sys.stderr)
    for match in noBlankLineAfterHeader.finditer(text):
        print((f'{sky_culture}: warning: no empty line after section header (see '
               f'https://www.markdownguide.org/basic-syntax/#heading-best-practices):\n'
               f'{match.group(1)}'), file=sys.stderr)

def update_descriptions_pot(sclist, pot):
    for sky_culture in sclist:
        data_path = os.path.join(SCDIR, sky_culture)
        description_file = os.path.join(data_path, 'description.md')
        assert os.path.exists(description_file)

        with open(description_file) as file:
            markdown = file.read()
            file.close()
            # First strip all the comments
            markdown = re.sub(r'<!--.*?-->', "", markdown)

            checkDescriptionSectionHeaders(sky_culture, markdown)

            titleRE = r'(?:^|\n)\s*# ([^\n]+)(?:\n|$)'
            titleL1 = re.findall(titleRE, markdown)
            if len(titleL1) != 1:
                print(f"{sky_culture}: There must be one level-1 title, instead found {len(titleL1)}: {titleL1}", file=sys.stderr)
                sys.exit(1)

            sc_name = titleL1[0]
            sc_names[sky_culture] = sc_name

            # First make cases with empty sections following each other with only a single line
            # in between separated by _two_ lines, so that the split below works in this case too.
            markdown = re.sub(r'\n[ \t]*##([^\n#][^\n]*)\n##([^#\n])', r'\n##\1\n\n##\2', markdown)

            sectionsL2 = re.split(r'\n[ \t]*##([^\n#][^\n]*)\n', markdown)
            if len(sectionsL2) < 3:
                print(f"{sky_culture}: Bad number of level-2 sections in description", file=sys.stderr)
                sys.exit(1)

            assert re.search(titleRE, sectionsL2[0])[0] == sectionsL2[0]
            assert len(sectionsL2) % 2 == 1 # The list is: [scName, sectionTitle, sectionBody, ..., sectionTitle, sectionBody]

            entry = polib.POEntry(comment = sc_name + " sky culture name", msgid = sc_name, msgstr = "")
            pot.append(entry)

            standard_l2_titles = ['Introduction', 'Description', 'Constellations', 'Extras', 'References', 'Authors', 'License']
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

                if sec_title == 'References':
                    checkReferences(sky_culture, sec_body)

                if sec_title == 'Constellations':
                    if handle_constellations_section(sky_culture, sc_name, sec_body, pot):
                        continue
                    else:
                        print(f'{sky_culture}: warning: no constellations could be extracted from Constellations '
                               'section, generating a legacy translation entry for the whole section', file=sys.stderr)

                comment = sc_name + ' sky culture ' + sec_title.lower() + ' section in markdown format'
                entry = polib.POEntry(comment = comment, msgid = sec_body, msgstr = "")
                if entry in pot:
                    prev_entry = pot.find(entry.msgid)
                    assert prev_entry
                    prev_entry.comment += '\n' + comment
                else:
                    pot.append(entry)

def update_cultures_pot(sclist, pot):
    def process_cons_or_asterism(array, obj_type, pot, sc_name):
        for obj in array:
            assert 'id' in obj
            obj_id = obj['id']

            obj_name = ''
            if 'common_name' in obj:
                name = obj['common_name']

                if 'english' in name:
                    english = name['english']
                    if len(english) == 0:
                        english = None
                else:
                    english = None

                if english and obj_name == '':
                    obj_name = english

                if 'native' in name:
                    native = name['native']
                    if len(native) == 0:
                        native = None
                else:
                    native = None

                if native and obj_name == '':
                    obj_name = native

                if 'pronounce' in name:
                    pronounce = name['pronounce']
                    if len(pronounce) == 0:
                        pronounce = None
                else:
                    pronounce = None

                if 'byname' in name:
                    byname = name['byname']
                    if len(byname) == 0:
                        byname = None
                else:
                    byname = None

                def get_comment(what_to_skip):
                    comment = f'{sc_name} {obj_type}'
                    if native and what_to_skip != 'native':
                        comment += f', native: {native}'

                    if pronounce and what_to_skip != 'pronounce':
                        comment += ', pronounce: ' + name['pronounce']

                    if english and what_to_skip != 'english':
                        comment += ', English: ' + name['english']

                    if byname and what_to_skip != 'byname':
                        comment += ', byname: ' + name['byname']

                    if 'translators_comments' in name:
                        comment += '\n' + name['translators_comments']
                    return comment

                context = None
                if 'context' in name:
                    context = name['context']

                # Extract 'english' string for translation (with context for uniqueness)
                if english:
                    if obj_type == 'constellation':
                        cons_names_for_describing[sky_culture].add(english)

                    # Don't extract items that are already translated in other places
                    if context or not english in common_names:
                        cons_ast_names.add(english)

                        entry = polib.POEntry(comment = get_comment('english'), msgid = english, msgstr = "", msgctxt = context)
                        if entry in pot:
                            prev_entry = pot.find(entry.msgid, msgctxt = context)
                            assert prev_entry
                            prev_entry.comment += '\n' + get_comment('english')
                        else:
                            pot.append(entry)
                else:
                    print(f'{sky_culture}: warning: common_name property in {obj_type} "{obj_id}" has no English name', file=sys.stderr)

                # Extract 'pronounce' string for translation (with context for uniqueness)
                if pronounce and not args.skip_pronounce:
                    # Don't extract items that are already translated in other places
                    if not pronounce in common_names:
                        cons_ast_names.add(pronounce)

                        entry = polib.POEntry(comment = "Pronounce entry for " + get_comment('pronounce'),
                                              msgid = pronounce, msgstr = "", msgctxt = context)
                        if entry in pot:
                            prev_entry = pot.find(entry.msgid, msgctxt = context)
                            assert prev_entry
                            prev_entry.comment += '\n' + get_comment('pronounce')
                        else:
                            pot.append(entry)
                #else:
                #    print(f'{sky_culture}: info: common_name property in {obj_type} "{obj_id}" has no pronounce element', file=sys.stderr)

                # Extract 'byname' string for translation (with context for uniqueness)
                if byname:
                    # Don't extract items that are already translated in other places
                    if not byname in common_names:

                        cons_ast_names.add(byname)

                        entry = polib.POEntry(comment = "Byname for " + get_comment('byname'), msgid = byname,
                                              msgstr = "", msgctxt = context)
                        if entry in pot:
                            prev_entry = pot.find(entry.msgid, msgctxt = context)
                            assert prev_entry
                            prev_entry.comment += '\n' + get_comment('byname')
                        else:
                            pot.append(entry)
                #else:
                #    print(f'{sky_culture}: info: common_name property in {obj_type} "{obj_id}" has no byname element', file=sys.stderr)
            else:
                print(f'{sky_culture}: warning: no common_name key in {obj_type} "{obj_id}"', file=sys.stderr)

            if not args.skip_abbrev:
                # process abbreviations of constellations and asterisms
                parts = obj_id.split(' ')
                abbr_comment = f'Abbreviation of {obj_type} in {sc_name} sky culture'
                if obj_name:
                    abbr_comment += f", name: {obj_name}"
                abbr_context = 'abbreviation'
                entry = polib.POEntry(comment = abbr_comment, msgid = parts[2], msgstr = "", msgctxt = abbr_context)
                if entry in pot:
                    prev_entry = pot.find(entry.msgid, msgctxt = abbr_context)
                    assert prev_entry
                    prev_entry.comment += '\n' + abbr_comment
                else:
                    pot.append(entry)

            if obj_name == '':
                obj_name = obj_id

            if 'description' in obj:
                desc = obj['description']
                comment = f'Description of {sc_name} {obj_type} {obj_name}'

                entry = polib.POEntry(comment = comment, msgid = desc, msgstr = "")
                if entry in pot:
                    prev_entry = pot.find(entry.msgid)
                    assert prev_entry
                    prev_entry.comment += '\n' + comment
                else:
                    pot.append(entry)


    def process_names(objects, pot, sc_name):
        for obj_id in objects:
            for name in objects[obj_id]:
                if 'english' in name:
                    english = name['english']
                    if len(english) == 0:
                        english = None
                else:
                    english = None

                if 'native' in name:
                    native = name['native']
                    if len(native) == 0:
                        native = None
                else:
                    native = None

                if 'pronounce' in name:
                    pronounce = name['pronounce']
                    if len(pronounce) == 0:
                        pronounce = None
                else:
                    pronounce = None

                if not english:
                    print(f'{sky_culture}: warning: common_name property in object "{obj_id}" has no English name', file=sys.stderr)
                    continue

                # Don't extract items that are already translated in other places
                if english in common_names:
                    continue

                chinese_name_cleaned = False
                if "chinese" in sc_name.lower():
                    cleaned = english

                    # Handle the names like "Royal Guards Added III" and "Celestial Hook Added IX*"
                    # TODO: also handle cases like "Celestial Farmland Added IV (In Horn Mansion)".
                    # Currently we don't handle the parenthesized part in the end. This change will
                    # also need a supporting change in the C++ code.
                    cleaned = re.sub(' Added [MDCLXVI]+[*?]*$', '', cleaned)
                    has_added = cleaned != english

                    # Handle the names like "Celestial Ramparts I" and "Persia VII*"
                    # TODO: similarly to the above, we also need to handle cases like
                    # "Pestle III (In Rooftop Mansion)" with a corresponding change in the C++ code.
                    cleaned = re.sub(' [MDCLXVI]+[*?]*$', '', cleaned)

                    chinese_name_cleaned = cleaned != english
                    english = cleaned

                    if has_added:
                        comment = ('This word is used in Chinese star names, e.g. "Wang Liang Added IX"\n'+
                                   'Note that the English source starts with a space. If you remove it in '+
                                   'your translation, you\'ll get no space between it and the preceding '+
                                   'word, something like "Wang LiangAdded IX" on the screen in Stellarium, '+
                                   'which may or may not be suitable for your locale.')
                        entry = polib.POEntry(msgid=' Added', comment=comment, msgctxt='chinese skycultures')
                        if entry not in pot:
                            pot.append(entry)

                    # Now the pronounce entries also need a lite version of the above cleaning
                    if pronounce:
                        pronounce = re.sub(' [MDCLXVI*?]+$', '', pronounce)

                comment = ''
                if not chinese_name_cleaned or not english in cons_ast_names:
                    if native:
                        comment = f'{sc_name} name for {obj_id}, native: {native}'
                    else:
                        comment = f'{sc_name} name for {obj_id}'

                if 'translators_comments' in name:
                    if len(comment) == 0:
                        comment += name['translators_comments']
                    else:
                        comment += '\n' + name['translators_comments']

                context = None
                if 'context' in name:
                    context = name['context']

                entry = polib.POEntry(comment = comment, msgid = english, msgstr = "", msgctxt = context)
                if entry in pot:
                    prev_entry = pot.find(entry.msgid, msgctxt = context)
                    assert prev_entry
                    if comment:
                        prev_entry.comment += '\n' + comment
                else:
                    pot.append(entry)

                # Extract 'pronounce' string for translation (with context for uniqueness)
                if pronounce and not args.skip_pronounce:
                    pcomment = f'Pronounce entry for {sc_name} name for {obj_id}'
                    if len(comment) > 0:
                        pcomment += '\n' + comment

                    entry = polib.POEntry(comment = pcomment, msgid = pronounce, msgstr = "", msgctxt = context)
                    if entry in pot:
                        prev_entry = pot.find(entry.msgid, msgctxt = context)
                        assert prev_entry
                        if comment:
                            prev_entry.comment += '\n' + pcomment
                    else:
                        pot.append(entry)

    def process_extra_names(objects, pot, sc_name):
        if 'context' in objects:
            context = objects["context"]
        else:
            context = None

        if 'comment' in objects:
            ecomment = objects["comment"]
        else:
            ecomment = None

        for name in objects["names"]:
            if 'english' in name:
                english = name['english']
                if len(english) == 0:
                    english = None
            else:
                english = None

            if not english:
                continue

            # pronounce is language dependent! The element is optional.
            if 'pronounce' in name:
                pronounce = name['pronounce']
                if len(pronounce) == 0:
                    pronounce = None
            else:
                pronounce = None

            def get_comment(for_what):
                comment = ''
                if for_what == 'english':
                    comment = 'Name of '
                elif for_what == 'pronounce':
                    comment = 'Pronounce entry for '
                comment += f'a zodiac sign or a lunar mansion in {sc_name} sky culture'

                if for_what != 'native' and 'native' in name:
                    comment += ', native: ' + name['native']
                if for_what != 'pronounce' and pronounce:
                    comment += ', pronounce: ' + pronounce
                if for_what != 'english' and 'english' in name:
                    comment += ', English: ' + name['english']
                if for_what != 'byname' and 'byname' in name:
                    comment += ', byname: ' + name['byname']
                if ecomment:
                    comment += '\n' + ecomment
                return comment

            comment = get_comment('english')
            entry = polib.POEntry(comment = comment, msgid = english, msgstr = "", msgctxt = context)
            if entry in pot:
                prev_entry = pot.find(entry.msgid, msgctxt = context)
                assert prev_entry
                if comment:
                    prev_entry.comment += '\n' + comment
            else:
                pot.append(entry)

            if not pronounce or args.skip_pronounce:
                continue

            comment = get_comment('pronounce')
            entry = polib.POEntry(comment = comment, msgid = pronounce, msgstr = "", msgctxt = context)
            if entry in pot:
                prev_entry = pot.find(entry.msgid, msgctxt = context)
                assert prev_entry
                if comment:
                    prev_entry.comment += '\n' + comment
            else:
                pot.append(entry)


    for sky_culture in sclist:
        data_path = os.path.join(SCDIR, sky_culture)
        index_file = os.path.join(data_path, 'index.json')
        assert os.path.exists(index_file)

        sc_name = sc_names[sky_culture]

        with open(index_file) as file:
            data = json.load(file)
            file.close()

            if 'constellations' in data:
                process_cons_or_asterism(data['constellations'], "constellation", pot, sc_name)
            if 'asterisms' in data:
                process_cons_or_asterism(data['asterisms'], "asterism", pot, sc_name)
            if 'zodiac' in data and not args.skip_zodiac:
                process_extra_names(data['zodiac'], pot, sc_name)
            if 'lunar_system' in data and not args.skip_zodiac:
                process_extra_names(data['lunar_system'], pot, sc_name)
            if 'common_names' in data:
                process_names(data['common_names'], pot, sc_name)

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("-s", "--sky-culture", help="Process only the specified sky culture")
    parser.add_argument("--skip-abbrev", action="store_true", help="Don't emit translations for abbreviations")
    parser.add_argument("--skip-zodiac", action="store_true", help="Don't emit translations for Zodiac or Lunar System entries")
    parser.add_argument("--skip-pronounce", action="store_true", help="Don't emit translations for 'pronounce' entries")
    args = parser.parse_args()
    metadata_template = {
        'Project-Id-Version': 'PACKAGE VERSION',
        'Report-Msgid-Bugs-To': 'stellarium@googlegroups.com',
        'Last-Translator': 'FULL NAME <EMAIL@ADDRESS>',
        'Language-Team': 'LANGUAGE <LL@li.org>',
        'Language': '',
        'MIME-Version': '1.0',
        'Content-Type': 'text/plain; charset=UTF-8',
        'Content-Transfer-Encoding': '8bit'
    }
    if args.sky_culture:
        sclist = [args.sky_culture]
        desc_pot = polib.POFile(encoding='utf-8', check_for_duplicates=True)
        desc_pot.metadata = metadata_template
        sc_pot = desc_pot # same file for everything
        desc_pot_file_path = None
        sc_pot_file_path = None
        common_pot_file_path = os.path.join(SCDIR, args.sky_culture, 'po', args.sky_culture+'.pot')
        ensure_dir(common_pot_file_path)
    else:
        sclist = [d for d in os.listdir(SCDIR) if is_sky_culture_dir(d)]
        sclist.sort()
        desc_pot = polib.POFile(encoding='utf-8', check_for_duplicates=True)
        desc_pot.metadata = metadata_template
        sc_pot = polib.POFile(encoding='utf-8', check_for_duplicates=True)
        sc_pot.metadata = metadata_template
        desc_pot_file_path = DESCPOTFILE
        sc_pot_file_path = SCPOTFILE
        common_pot_file_path = None

    # Enumerate all constellation names in all SCs, they will be needed for sanity checks in description parser
    print("Enumerating constellation names...", file=sys.stderr)
    for sky_culture in sclist:
        data_path = os.path.join(SCDIR, sky_culture)
        index_file = os.path.join(data_path, 'index.json')
        assert os.path.exists(index_file)
        cons_names_for_describing[sky_culture] = set()
        with open(index_file) as file:
            data = json.load(file)
            file.close()
            if 'constellations' in data:
                for cons in data['constellations']:
                    if 'common_name' in cons:
                        name = cons['common_name']
                        if 'english' in name:
                            english = name['english']
                            if len(english) == 0:
                                continue
                            cons_names_for_describing[sky_culture].add(english)

    print("Loading common names...", file=sys.stderr)
    load_common_names()
    print("Updating descriptions .pot file...", file=sys.stderr)
    update_descriptions_pot(sclist, desc_pot)
    if desc_pot_file_path:
        desc_pot.save(desc_pot_file_path)
    print("Updating SC index .pot file...", file=sys.stderr)
    update_cultures_pot(sclist, sc_pot)
    if sc_pot_file_path:
        sc_pot.save(sc_pot_file_path)
    if common_pot_file_path:
        sc_pot.save(common_pot_file_path)
