#!/usr/bin/python
#
# Updates the translationdata.js in the data/webroot folder using the current stellarium-remotecontrol.jst file

import re
import sys

def main():
	'''
	main expects 2 arguments:
	'''

	if len(sys.argv) < 3:
		print("usage: update_translationdata.py <stellarium-remotecontrol.jst> <translationdata.js>")
		print(sys.argv)
		return
	potfile = open(sys.argv[1], 'r')
	jsfile = open(sys.argv[2], 'w')
	
	p = re.compile(r'^msgid "([^"\\]*(?:\\.[^"\\]*)*)"$', re.MULTILINE)
	
	jsfile.write(
	"""
//This file is generated automatically by the stellarium-remotecontrol-update-translationdata target through update_translationdata.py from stellarium-remotecontrol.jst
//It contains all strings that can be translated through the StelTranslator in the JavaScript code by calling Main.tr()
//When this file is requested through the RemoteControl web server, the strings are translated using the current Stellarium app language

define({
	""")
	
	first = True
	
	for i in p.finditer(potfile.read()):
		if i.group(1):
			if not first:
				jsfile.write(',\n\t')
			jsfile.write('"'+i.group(1).replace('"','\\"')+'" : \'<?= tr("'+i.group(1).replace("'","\\'")+'")?>\'')
			first = False
	jsfile.write("\n});")

if __name__ == '__main__':
	main()
