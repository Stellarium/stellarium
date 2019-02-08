#!/usr/bin/env python
# -*- coding: utf-8 -*-

import errno, glob, polib, re, os, getopt, sys
from time import strftime

def usage():
    print '\nUsage: python %s [OPTION]' %os.path.basename(sys.argv[0])
    print '       extract existing translations from desktop file'
    print 'Options: -h, --help                              : usage'
    sys.exit(2)
try:
    opts, args = getopt.getopt(sys.argv[1:], "h:", ["help"])
except getopt.GetoptError:
    usage() # print help information and exit

directory='../../data/'
for o in opts:
    if o in ("-h", "--help"):
        usage()

# Find all desktop files
files = []
for rootdir, dirnames, filenames in os.walk(directory):
    files.extend(glob.glob(rootdir + "/*.desktop"))

# Define Templates and po directory name
translationtemplate='(?<=\n)Name\[.*?\n|GenericName\[.*?\n|Comment\[.*?\n'
tpattern=re.compile(translationtemplate,re.DOTALL)
podir = '../../po/stellarium-desktop'

pocreationtime = strftime('%Y-%m-%d %H:%M%z')

for langfile in files:
  langfiledir = langfile.replace('.desktop', '')
  langfilename = langfiledir.rpartition('/')[2]
  # Create localization directories if needed
  try:
    os.makedirs(podir)
  except OSError, e:
    if e.errno != errno.EEXIST:
        raise
  #open desktop file
  text = open(langfile,"r").read()

  # Parse contents and add them to PO
  for tblock in tpattern.findall(text):
    message_comment, locale_message = tblock.strip('\n').split('[')
    lang_code, msg_str = locale_message.split(']=')
    msgidtemplate='(?<=\n)' + message_comment + '=.*?\n'
    msgidpattern=re.compile(msgidtemplate,re.DOTALL)
    msgids = msgidpattern.findall(text)
    if msgids:
      msg_id = msgids[0].split('=')[1].strip('\n')
      poentry = polib.POEntry(
	msgctxt = message_comment,
	msgid = msg_id.decode('utf-8'),
	msgstr = msg_str.decode('utf-8'),
	occurrences=[(langfile,'')]
	)
      pofilename = podir + '/' + lang_code + '.po'
      if not os.path.isfile(pofilename):
	# Create PO file
	po = polib.POFile()
	po.metadata = {
	  'Project-Id-Version': 'Stellarium desktop file translation',
	  'Report-Msgid-Bugs-To': 'stellarium-translation@lists.launchpad.net',
	  'POT-Creation-Date': pocreationtime,
	  'PO-Revision-Date': pocreationtime,
	  'Last-Translator': 'FULL NAME <EMAIL@ADDRESS>',
	  'Language-Team': lang_code + ' <' + lang_code + '@li.org>',
	  'MIME-Version': '1.0',
	  'Content-Type': 'text/plain; charset=UTF-8',
	  'Content-Transfer-Encoding': '8bit',
	  }
	po.save(pofilename)
      po = polib.pofile(pofilename, check_for_duplicates=True)
      if msg_id != '':
	try:
	  po.append(poentry)
	except ValueError:
	  print 'The entry already exists, skipping it'
      po.save(pofilename)
