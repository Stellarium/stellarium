#!/usr/bin/python3

# Stellarium
# Copyright (C) 2022 - Stellarium Labs SRL
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
import shutil
import sys
import zipfile
import tempfile
import urllib.request
import json
import polib
import argparse

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

LANGS = ['aa','ae','af','am','ar','ast','av','az','be','bg','bh','bi','bn',
         'br','bs','ca','ce','cs','csb','cv','da','de','el','en','en_AU',
         'en_CA','en_GB','en_US','eo','es','et','eu','fa','fi','fil','fj','fr',
         'fy','ga','gd','gl','gn','gu','gv','he','hi','hr','hrx','hu','hy',
         'id','is','it','ja','jv','ka','kg','kk','ko','ku','ky','la','lt','lv',
         'mk','ml','mn','mo','mr','ms','na','nb','nds','ne','nl','nn','oj',
         'pa','pl','pt','pt_BR','ro','ru','sc','si','sk','sl','sq','sr','sv',
         'sw','ta','te','tg','th','tl','tr','ug','uk','ur','vi','zh','zh_CN',
         'zh_HK','zh_TW']

BLACKLIST = [
    'northern_andes',  # Crappy quality
    # Un-clear images licencing
    'aztec',
    'boorong',
    'dakota',
    'macedonian',
    'ojibwe',
    'lokono',
    # Too many similar Arabic SCs
    'arabic_arabian_peninsula'
]


def ensure_dir(file_path):
    '''Create a directory for a path if it doesn't exist yet'''
    directory = os.path.dirname(file_path)
    if not os.path.exists(directory):
        os.makedirs(directory)


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
    OUT_DIR_I18N = os.path.join(DIR, '..', 'po', 'stellarium-skycultures')
    ensure_dir(os.path.join(OUTSCDIR, 'file'))
    ensure_dir(os.path.join(OUT_DIR_I18N, 'file'))

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

        print(f'Processing {sky_culture} from {index_file}')
        # Delete the previous data and copy all culture data into the out
        # directory.
        out_dir = os.path.join(OUTSCDIR, sky_culture)
        shutil.rmtree(out_dir, ignore_errors=True)
        ensure_dir(out_dir)
        shutil.copytree(data_path, out_dir)
        shutil.copy(os.path.join(OUTSCDIR, 'CMakeLists.template'),
                    os.path.join(out_dir, 'CMakeLists.txt'))
        out_po_dir = os.path.join(out_dir, 'po')
        shutil.rmtree(out_po_dir, ignore_errors=True)
        out_doc_dir = os.path.join(out_dir, 'doc')
        shutil.rmtree(out_doc_dir, ignore_errors=True)

    # Generates combined po files to obtain 1 po file with all sky cultures
    # strings per supported language.
    print('Generating combined po files')
    for lang in LANGS:
        input_lang = lang
        if lang == 'zh_Hant':
            input_lang = 'zh_TW'
        if lang == 'zh_Hans':
            input_lang = 'zh_CN'

        combined_po = polib.POFile(encoding='utf-8', check_for_duplicates=True)
        combined_po.metadata = {
            'Project-Id-Version': '1.0',
            'Last-Translator': "Stellarium Labs",
            'Language-Team': "Stellarium Labs",
            'MIME-Version': '1.0',
            'Content-Type': 'text/plain; charset=utf-8',
            'Content-Transfer-Encoding': '8bit',
            'Language': lang
        }

        for sky_culture in sclist:
            if sky_culture == 'out':
                continue
            po_path = os.path.join(SCDIR, sky_culture, 'po', input_lang + '.po')
            if not os.path.exists(po_path):
                print('Warning: no language "'+input_lang+'" for sky culture "'+sky_culture+'"')
                continue
            po = polib.pofile(po_path)
            for entry in po.translated_entries():
                if entry not in combined_po:
                    combined_po.append(entry)
            for entry in po.untranslated_entries():
                if entry not in combined_po:
                    combined_po.append(entry)
        if len(combined_po.translated_entries()) == 0:
            print('Warning: no strings present for language "'+input_lang+'"')
            continue

        combined_po_path = os.path.join(OUT_DIR_I18N, lang + '.po')
        combined_po.save(combined_po_path)


if __name__ == '__main__':
    main()
