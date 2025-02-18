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

# Script that updates sky cultures data from official github project

import os
import re
import sys
import json
import polib
import shutil
import zipfile
import tempfile
import argparse
import urllib.request

if sys.version_info[0] < 3:
    raise Exception("Please use Python 3 (current is {})".format(
        sys.version_info[0]))

if os.path.dirname(os.path.relpath(__file__)) != 'skycultures':
    print("This script must be run from the skycultures/ directory")
    sys.exit(-1)

DIR = os.path.abspath(os.path.dirname(__file__))

parser = argparse.ArgumentParser()
parser.add_argument("--sky_culture_dir",
                    help="directory where sky cultures are stored.")
args = parser.parse_args()

LANGS = ['af','am','ar','arn','ay','az','be','bg','bn','br','bs','ca',
         'ca@valencia','cs','da','de','el','en','en_AU','en_CA','en_GB','en_US',
         'es','es_419','et','eu','fa','fi','fr','fy','gd','gl','gu','he','hi',
         'hr','hu','hy','id','is','it','ja','jv','ka','ko','la','lb','lt','lv',
         'mi','ml','mr','ms','nb','nds','ne','nl','nn','oj','pl','pt','pt_BR',
         'qu','rap','ro','ru','sa','sc','shi','si','sk','sl','sq','sr','sr@latin',
         'sv','ta','te','th','ti','tr','tt','uk','vi','zh_CN','zh_HK','zh_TW']

BLACKLIST = [
    # Unclear images licensing
    'dakota',
    'ojibwe',
]

sc_names = {}

def ensure_dir(file_path):
    '''Create a directory for a path if it doesn't exist yet'''
    directory = os.path.dirname(file_path)
    if not os.path.exists(directory):
        os.makedirs(directory)

def is_sc_descr_entry(entry, sc_name):
    '''Check if a given .po file entry is a section from a sky culture description'''
    if not entry.comment:
        return False
    c = entry.comment
    if not c.startswith('Sky culture ') and not c.startswith(sc_name+' sky culture '):
        return False
    if 'Sky culture name' in c or (sc_name+' sky culture name') in c:
        return True
    return c.endswith(' section in markdown format')

def main():
    SCDIR = None
    if args.sky_culture_dir:
        # The user passed the path to the sky cultures directory
        SCDIR = args.sky_culture_dir
    else:
        # If not path is given, get the data from upstream github
        url = 'https://github.com/Stellarium/stellarium-skycultures/archive/master.zip'
        path = '/tmp/stellarium-skycultures-master.zip'
        print(f'Download {url}')
        urllib.request.urlretrieve(url, path)
        outpath = tempfile.mkdtemp()

        with zipfile.ZipFile(path, 'r') as zip_ref:
            zip_ref.extractall(outpath)
        SCDIR = os.path.join(outpath, 'stellarium-skycultures-master')
    OUTSCDIR = os.path.join(DIR)
    OUT_DIR_I18N_SC = os.path.join(DIR, '..', 'po', 'stellarium-skycultures')
    OUT_DIR_I18N_SC_BACKUP = os.path.join(DIR, '..', 'po', 'stellarium-sky')
    OUT_DIR_I18N_GUI = os.path.join(DIR, '..', 'po', 'stellarium-skycultures-descriptions')
    ensure_dir(os.path.join(OUTSCDIR, 'file'))
    ensure_dir(os.path.join(OUT_DIR_I18N_SC, 'file'))
    ensure_dir(os.path.join(OUT_DIR_I18N_GUI, 'file'))

    def is_sky_culture_dir(d):
        if not os.path.isdir(os.path.join(SCDIR, d)):
            return False
        if d in BLACKLIST:
            return False
        index_file = os.path.join(SCDIR, d, 'index.json')
        return os.path.exists(index_file)

    # Remove blacklisted SCs
    sclist = [d for d in os.listdir(SCDIR) if d in BLACKLIST]
    for sky_culture in sclist:
        out_dir = os.path.join(OUTSCDIR, sky_culture)
        shutil.rmtree(out_dir, ignore_errors=True)

    sclist = [d for d in os.listdir(SCDIR) if is_sky_culture_dir(d)]
    sclist.sort()

    for sky_culture in sclist:
        data_path = os.path.join(SCDIR, sky_culture)
        index_file = os.path.join(data_path, 'index.json')
        assert os.path.exists(index_file)
        description_file = os.path.join(data_path, 'description.md')
        assert os.path.exists(description_file)

        print(f'Processing {sky_culture} from {index_file}')

        with open(description_file) as file:
            markdown = file.read()
            file.close()
            # First strip all the comments
            markdown = re.sub(r'<!--.*?-->', "", markdown)

            # Now note the English name of the SC
            titleRE = r'(?:^|\n)\s*# ([^\n]+)\n'
            titleL1 = re.findall(titleRE, markdown)
            if len(titleL1) != 1:
                print(f"{sky_culture}: There must be one level-1 title, instead found {len(titleL1)}: {titleL1}", file=sys.stderr)
                sys.exit(1)
            sc_name = titleL1[0]
            sc_names[sky_culture] = sc_name

        # Delete the previous data and copy all culture data into the out
        # directory.
        out_dir = os.path.join(OUTSCDIR, sky_culture)
        shutil.rmtree(out_dir, ignore_errors=True)
        ensure_dir(out_dir)
        shutil.copytree(data_path, out_dir)
        shutil.copy(os.path.join(OUTSCDIR, 'CMakeLists.txt.template'),
                    os.path.join(out_dir, 'CMakeLists.txt'))
        out_po_dir = os.path.join(out_dir, 'po')
        shutil.rmtree(out_po_dir, ignore_errors=True)
        out_doc_dir = os.path.join(out_dir, 'doc')
        shutil.rmtree(out_doc_dir, ignore_errors=True)

    # Generates combined po files to obtain 1 po file with all sky cultures
    # strings per supported language.
    print('Generating combined po files')
    for lang in LANGS:
        print('Processing language "'+lang+'"...')
        input_lang = lang
        if lang == 'zh_Hant':
            input_lang = 'zh_TW'
        if lang == 'zh_Hans':
            input_lang = 'zh_CN'

        combined_sc_po = polib.POFile(encoding='utf-8', check_for_duplicates=True)
        sc_po_path = os.path.join(OUT_DIR_I18N_SC, lang + '.po')
        sc_po_backup_path = os.path.join(OUT_DIR_I18N_SC_BACKUP, lang + '.po')
        if os.path.exists(sc_po_path):
            orig_sc_po = polib.pofile(sc_po_path)
            combined_sc_po.metadata = orig_sc_po.metadata
        elif os.path.exists(sc_po_backup_path):
            orig_sc_po = polib.pofile(sc_po_backup_path)
            combined_sc_po.metadata = orig_sc_po.metadata
        else:
            print(f"Warning: no original file {sc_po_path}. Please check the metadata after this script finishes.", file=sys.stderr)
            combined_sc_po.metadata = {
                'Project-Id-Version': '1.0',
                'Last-Translator': "(unknown)",
                'Language-Team': "(unknown)",
                'MIME-Version': '1.0',
                'Content-Type': 'text/plain; charset=UTF-8',
                'Content-Transfer-Encoding': '8bit',
                'Language': lang
            }

        combined_gui_po = polib.POFile(encoding='utf-8', check_for_duplicates=True)
        gui_po_path = os.path.join(OUT_DIR_I18N_GUI, lang + '.po')
        if os.path.exists(gui_po_path):
            orig_gui_po = polib.pofile(gui_po_path)
            combined_gui_po.metadata = orig_gui_po.metadata
        else:
            print(f"Warning: no original file {gui_po_path}. Please check the metadata after this script finishes.", file=sys.stderr)
            combined_gui_po.metadata = {
                'Project-Id-Version': '1.0',
                'Last-Translator': "(unknown)",
                'Language-Team': "(unknown)",
                'MIME-Version': '1.0',
                'Content-Type': 'text/plain; charset=UTF-8',
                'Content-Transfer-Encoding': '8bit',
                'Language': lang
            }

        setattr(combined_sc_po, 'check_for_duplicates', False)
        setattr(combined_gui_po, 'check_for_duplicates', False)

        for sky_culture in sclist:
            if sky_culture == 'out':
                continue
            po_path = os.path.join(SCDIR, sky_culture, 'po', input_lang + '.po')
            if not os.path.exists(po_path):
                print('Warning: no language "'+input_lang+'" for sky culture "'+sc_names[sky_culture]+'"')
                continue
            po = polib.pofile(po_path)
            for entry in po:
                if is_sc_descr_entry(entry, sc_names[sky_culture]):
                    if entry not in combined_gui_po:
                        entry.comment = sc_names[sky_culture] + ' s' + entry.comment[1:]
                        combined_gui_po.append(entry)
                else:
                    if entry not in combined_sc_po:
                        if entry.comment.startswith("Name for "):
                            entry.comment = sc_names[sky_culture] + ' n' + entry.comment[1:]
                        combined_sc_po.append(entry)

        if len(combined_sc_po.translated_entries()) == 0:
            print('Warning: no name strings present for language "'+input_lang+'"')
            continue
        if len(combined_sc_po.translated_entries()) == 0:
            print('Warning: no description strings present for language "'+input_lang+'"')
            continue

        combined_sc_po.save(sc_po_path)
        combined_gui_po.save(gui_po_path)


if __name__ == '__main__':
    main()
