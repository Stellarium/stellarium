#!/usr/bin/python
#
# Updates the translationdata.js in the data/webroot folder using the current stellarium-remotecontrol.jst file

import re
import sys
from pathlib import Path


def main():
    '''
    main expects 2 arguments: <stellarium-remotecontrol.jst> <translationdata.js>
    '''

    if len(sys.argv) < 3:
        print("usage: update_translationdata.py <stellarium-remotecontrol.jst> <translationdata.js>")
        print(sys.argv)
        return

    jst_file = Path(sys.argv[1])
    js_file = Path(sys.argv[2])

    if not jst_file.exists():
        print(f"'{jst_file}' do not exist.")
        return
    if not js_file.exists():
        print(f"'{js_file}' do not exist.")
        return

    msg_pattern = re.compile(r'^msgid "([^"\\]*(?:\\.[^"\\]*)*)"$', re.MULTILINE)

    with open(js_file, mode='w', encoding='utf-8') as js_writer:
        js_writer.write(
            """//This file is generated automatically by the stellarium-remotecontrol-update-translationdata target through update_translationdata.py from stellarium-remotecontrol.jst
//It contains all strings that can be translated through the StelTranslator in the JavaScript code by calling Main.tr()
//When this file is requested through the RemoteControl web server, the strings are translated using the current Stellarium app language

define({
""")
        with open(jst_file, mode='r', encoding='utf-8') as jst_reader:
            for find in msg_pattern.finditer(jst_reader.read()):
                if find.group(1):
                    key = find.group(1).replace('"', '\\"')
                    value = find.group(1).replace("'", "\\'")
                    js_writer.write(f'\t"{key}" : \'<?= tr("{value}")?>\',\n')
        js_writer.write("});")


if __name__ == '__main__':
    main()
