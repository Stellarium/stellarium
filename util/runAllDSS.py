#!/usr/bin/python
#
# Simple script which convert the FITS headers associated to Brian McLean DSS images into simplified JSON files
#
# (C) Fabien Chereau <fchereau@eso.org>
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

import sys
import os
import subprocess

def all():
	for i in range(1,895):
		str = "/home/fab1/prog/stellarium/util/dssheaderToJSON.py" + " S %i %i" % (i,i+1)
		print str
		try:
			retcode = subprocess.call(str, shell=True)
			if retcode < 0:
				print >>sys.stderr, "Child was terminated by signal", -retcode
			else:
				print >>sys.stderr, "Child returned", retcode
		except OSError, e:
			print >>sys.stderr, "Execution failed:", e

if __name__ == "__main__":
    all()
