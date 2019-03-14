#!/usr/bin/python
#
# Simple script which convert the FITS headers associated to Brian McLean DSS images into simplified JSON files
# Fabien Chereau fchereau@eso.org
#

import sys
import os
import subprocess

def allDSS():
    for i in range(1,895):
        str = "/home/fab1/prog/stellarium/util/dssheaderToJSON.py" + " S %i %i" % (i,i+1)
        print(str)
        try:
            retcode = subprocess.call(str, shell=True)
            if retcode < 0:
                print(f"Child was terminated by signal {retcode}", file=sys.stderr)
            else:
                print("Child returned {retcode}", file=sys.stderr)
        except OSError as e:
            print(f"Execution failed: {e}", file=sys.stderr)

if __name__ == "__main__":
    allDSS()
