#!/usr/bin/python

# This file is part of Stellarium. Stellarium is free software: you can
# redistribute it and/or modify it under the terms of the GNU General Public
# License as published by the Free Software Foundation, version 2.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc., 51
# Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#
# Copyright Fabien Chereau <fabien.chereau@gmail.com>

import json
import os
from multiprocessing import Pool

import Image
import numpy
from astropy import wcs
from astropy.io import fits

import dssUtils

# -----------------------------------------------------------
# Pre-process zipped plates to extract only necessary data
# This has to be run once if you don't already have the
# preprocessed plates
# -----------------------------------------------------------
# python prepareAllPlates.py
#
# input : a directory with all original plates .tgz files (change in code)
# output: a directory "preparedPlates" containing for each plate a single full resolution jpg image + a FITS headers necessary for sky-to-pixel projection
# usage : you must change the location of your input DSS plates path directly in the script

# The directory containing the plates .tgz files
originalPlatesDirectory = "/media/fabien/data/DSS-orig"


# Pre-compute meta-infos about the plate as well as a re-combination of all tile into one large image
def preparePlate(plateName):
    if not os.path.exists("preparedPlates"):
        os.system("mkdir preparedPlates")

    if os.path.exists("preparedPlates/%s-x64-FITS-header.hhh" % plateName):
        print "%s already processed, skip." % plateName
        return

    print "-------- " + plateName + " --------"
    if os.path.exists("preparedPlates/%s" % (plateName)):
        print "Plate %s already pre-processed" % (plateName)
        return
    print "Copy and prepare plate data"
    os.system("cp %s/%s.tgz ." % (originalPlatesDirectory, plateName))
    os.system("tar -xzf %s.tgz" % plateName)
    os.system("chmod -R a+w %s" % plateName)
    assert os.path.exists("%s" % plateName)

    print "create full plate image"
    resImg = Image.new("RGB", (64 * 300, 64 * 300))
    for j in range(0, 64):
        for i in range(0, 64):
            if os.path.exists("%s/x1/%s_%.2d_%.2d_x1.jpg" % (plateName, plateName, i, j)):
                im = Image.open("%s/x1/%s_%.2d_%.2d_x1.jpg" % (plateName, plateName, i, j))
                resImg.paste(im, (i * 300, 64 * 300 - j * 300 - 300))
            else:
                print "Missing jpeg tile: %s/x1/%s_%.2d_%.2d_x1.jpg" % (plateName, plateName, i, j)

    print "package and store results"
    os.system("mkdir preparedPlates/%s" % plateName)
    resImg.save("preparedPlates/%s/%s.jpg" % (plateName, plateName))
    os.system("cp %s/x64/%s_00_00_x64.hhh preparedPlates/%s/%s_00_00_x64-FITS-header.hhh" % (
    plateName, plateName, plateName, plateName))

    wcsWithoutCorrection = dssUtils.DssWcs(plateName)

    print "Compute the array of pixel offsets needed to adjust WCS conversions."
    # Load the x1 FITS headers to get the corrective offset term
    allOffsets = []

    for i in range(0, 64, 4):
        for j in range(0, 64, 4):
            if not os.path.exists("%s/x1/%s_%.2d_%.2d_x1.hhh" % (plateName, plateName, i, j)):
                print "Missing .hhh file"
                continue
            hdulist = fits.open("%s/x1/%s_%.2d_%.2d_x1.hhh" % (plateName, plateName, i, j))
            w = wcs.WCS(hdulist[0].header)
            arr = numpy.array([[0., 0.]], numpy.float_)  # Bottom left corner
            radecCorner = w.wcs_pix2world(arr, 1)
            realPixelPos = [i * 300, j * 300]
            currPixelPos = wcsWithoutCorrection.raDecToPixel([radecCorner[0][0], radecCorner[0][1]])
            assert currPixelPos != None
            allOffsets.append({'pixelPos': currPixelPos,
                               'offset': [realPixelPos[0] - currPixelPos[0], realPixelPos[1] - currPixelPos[1]]})

    with open("preparedPlates/%s/%s.offsets" % (plateName, plateName), 'w') as f:
        f.write(json.dumps(allOffsets))

    print "clean up"
    os.system("rm %s.tgz" % plateName)
    os.system("rm -rf %s" % plateName)


jobsArgs = dssUtils.getAllPlatesNames()()

pool = Pool(2)
try:
    pool.map_async(preparePlate, jobsArgs).get(9999999)
except KeyboardInterrupt:
    print "Caught KeyboardInterrupt, terminating workers"
    pool.terminate()
    pool.join()
