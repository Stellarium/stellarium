#!/usr/bin/python
# Simple script which convert the FITS headers associated to Brian McLean DSS images into simplified JSON files
# Fabien Chereau fchereau@eso.org

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
