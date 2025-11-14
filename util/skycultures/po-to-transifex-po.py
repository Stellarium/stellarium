#!/usr/bin/python3
#
# This script tries to convert the po to the dialect emitted by
# Transifex. This mainly affects trailing whitespace in comments
# and the rules of string wrapping.

import re
import polib
from pathlib import Path
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("filename", help="Translations file to process")
args = parser.parse_args()
pofile = polib.pofile(args.filename)
pofile.header = '\n' + pofile.header
pofile.save(args.filename)

# Fixup some minor differences in the header
path = Path(args.filename)
text = path.read_text()
metadata_end = text.find('\n\n')
metadata = text[:metadata_end]
metadata = re.sub(r'(^|\n)#\n', r'\1# \n', metadata)
metadata = re.sub((r'\n"Language: ([^"]+)\\n"'
               r'\n"MIME-Version: ([^"]+)\\n"'
               r'\n"Content-Type: ([^"]+)\\n"'
               r'\n"Content-Transfer-Encoding: ([^"]+)\\n"'),
              (r'\n"MIME-Version: \2\\n"'
               r'\n"Content-Type: \3\\n"'
               r'\n"Content-Transfer-Encoding: \4\\n"'
               r'\n"Language: \1\\n"'),
              metadata)
text = metadata + text[metadata_end:]
path.write_text(text)
