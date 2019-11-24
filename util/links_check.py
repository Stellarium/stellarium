#!/usr/bin/python
# encoding= utf-8

# Check for broken links in html data files.
#
# Usage:
#
#   links_check [DIR]
#
# If DIR is specified, look only for files in DIR, otherwise, look for all
# the files.
#
# The script make a HEAD http request for each link found, and print a
# message in case of an error.


# Copyright (C) 2016 Guillaume Chereau
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
# Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA 
# 02110-1335, USA.


import fnmatch
import os
import sys

import requests
from bs4 import BeautifulSoup

urls = set()
sources = {}

# Get the list of all files matching '*.utf8'
base = "./"
if len(sys.argv) == 2:
    base = sys.argv[1] + "/"

files = []
for root, dirnames, filenames in os.walk(base):
    for filename in fnmatch.filter(filenames, '*.utf8'):
        files.append(os.path.join(root, filename))

print "Got %d files" % len(files)

# Retreive all the http links.
for f in files:
    soup = BeautifulSoup(open(f))
    for link in soup.findAll('a'):
        url = link.get('href')
        if not url or url.startswith('mailto') or url.startswith('#'):
            continue
        url = unicode(url)
        urls.add(url)
        sources.setdefault(url, set()).add(f)

print "Got %d urls" % len(urls)
print

# Test each link one by one.
for url in urls:
    try:
        # We ignore the 301 return code, even though it would be nice to
        # replace the links with the new url.
        r = requests.head(url, allow_redirects=True)
        s = r.status_code
        if s == 200:
            continue
    except Exception:
        s = u"err"
    print url.encode('utf-8')
    print s
    print "Found in:"
    for f in sources[url]:
        print f
    print
    sys.stdout.flush()
