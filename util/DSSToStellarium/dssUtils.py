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

import Image
import numpy
import scipy.interpolate
from astropy import wcs
from astropy.io import fits


def getAllPlatesNames():
    """Return the list of all DSS plates names"""
    ret = []
    ret = ["S%03d" % i for i in range(1, 895)]
    for i in range(2, 899):
        ret.append("N%03d" % i)
    return ret


def getTestingPlatesNames():
    """Return a list of some testing DSS plates names"""
    ret = ["N%03d" % i for i in range(100, 305)]
    return ret


def getTestingPlatesNamesLMC():
    """Return a list of some testing DSS plates names covering the Large Magellanic Cloud (one of the worst case)"""
    return ["S086", "S085", "S057", "S056", "S033", "S032", "S055", "S054"]


def getValidRegionForPlate(plateName):
    """Return the polygon defining the valid pixels inside the full resolution plate image"""
    if plateName[0] == 'N':
        return [[0, 0], [17232, 0], [17232, 1200], [19200, 1200], [19200, 19200], [0, 19200]]
    else:
        assert plateName[0] == 'S'
        return [[480, 0], [17232, 0], [17232, 17232], [0, 17232], [0, 624], [480, 624]]


def enhancePlate(plateName, plate, referenceImage):
    """Return a transformed version of the plate so that it matches the referenceImage
    This is Fabien's version. It just attempts to scale brightness based on the average
    brightness on a 8x8 map"""

    print "Enhance darker part of the image"
    plate = plate.point(lambda t: 2. * t - 256. * (t / 256.) ** 1.8)

    """
    plate.save("tmp/%s-full.jpg" % plateName, 'JPEG', quality=80)
    referenceImage.save("tmp/%s-reference.jpg" % plateName, 'JPEG', quality=95)
    return plate
    
    print "Enhance plate"
    plateMap = plate.resize((8,8) , Image.ANTIALIAS)
    plateMap.save("tmp/%s-full.jpg" % plateName, 'JPEG', quality=95)
    
    referenceImageMap = referenceImage.resize((8,8) , Image.ANTIALIAS)
    referenceImageMap.save("tmp/%s-reference.jpg" % plateName, 'JPEG', quality=95)
    
    diffMap = plateMap-referenceImageMap
    
    step=8./plate.size[0]
    
    platePix = plate.load()
    for i in range(plate.size[0]):
        for j in range(plate.size[1]):
            platePix[i, j] = platePix[i, j]+i*step
    
    print "Done enhancing plate"
    """
    return plate


def enhancePlate2(plateName, plate, referenceImage):
    """Return a transformed version of the plate so that it matches the referenceImage"""
    # print "Enhance darker part of the image"
    plate = plate.point(lambda t: 2. * t - 256. * (t / 256.) ** 1.8)

    enhanceScaling = 32

    print "Enhance plate"
    plate = plate.resize((256, 256), Image.BICUBIC)
    plate.save("tmp/%s-full.jpg" % plateName, 'JPEG', quality=95)
    # referenceImage = referenceImage.resize(plate.size, Image.BILINEAR)
    referenceImage.save("tmp/%s-reference.jpg" % plateName, 'JPEG', quality=95)

    plate = None
    # referenceImage = None

    # os.system("python src/main.py --hi_res tmp/%s-full.jpg --lo_res tmp/%s-reference.jpg --output tmp/%s-enhanced.jpg" % (plateName, plateName, plateName) )
    referenceImage.save("tmp/%s-enhanced.jpg" % plateName, 'JPEG', quality=95)

    plate = Image.open("tmp/%s-enhanced.jpg" % plateName)
    plate = plate.resize((plate.size[0] * enhanceScaling, plate.size[0] * enhanceScaling), Image.BILINEAR)
    print "Done enhancing plate"

    return plate


class DssWcs:
    """A pixel to sky conversion class. It wraps a WCS object read from the FITS header"""

    def __init__(self, plateName):
        # Load the FITS hdulist using astropy.io.fits
        assert os.path.exists("preparedPlates/")
        hdulist = fits.open("preparedPlates/%s/%s_00_00_x64-FITS-header.hhh" % (plateName, plateName))
        self.w = wcs.WCS(hdulist[0].header)

        try:
            f = open("preparedPlates/%s/%s.offsets" % (plateName, plateName), 'r')
        except IOError:
            self.gridPixelPos = None
            self.gridOffset = None
            self.inverseGridPixelPos = None
        else:
            offsets = json.loads(f.read())
            f.close()

            self.gridPixelPos = []
            self.gridOffset = []
            self.inverseGridPixelPos = []
            for el in offsets:
                self.gridPixelPos.append(el['pixelPos'])
                self.gridOffset.append(el['offset'])
                self.inverseGridPixelPos.append(
                    [el['pixelPos'][0] + el['offset'][0], el['pixelPos'][1] + el['offset'][1]])
            self.gridPixelPos = numpy.array(self.gridPixelPos)
            self.gridOffset = numpy.array(self.gridOffset)
            self.inverseGridPixelPos = numpy.array(self.inverseGridPixelPos)

    def pixelToRaDec(self, pixelPos):
        """Return the RA DEC pair for a given pixel position (even if outside the plate)
        the passed wcs must be the one laying in the preparedPlates directory"""
        arr = [pixelPos[0], pixelPos[1]]
        if self.inverseGridPixelPos != None:
            nearestOffset = scipy.interpolate.griddata(self.inverseGridPixelPos, self.gridOffset, [arr],
                                                       method='nearest')
            arr = [arr[0] - nearestOffset[0][0], arr[1] - nearestOffset[0][1]]

        arr = [arr[0] / 64. + 1.5, arr[1] / 64. + 1.5]
        arr = numpy.array([arr], numpy.float_)
        ret = self.w.wcs_pix2world(arr, 1)
        return [ret[0][0], ret[0][1]]

    def raDecToPixel(self, raDecPos):
        """Return the pixel pos for a given RA DEC sky position (even if outside the plate)
        the passed wcs must be the one laying in the preparedPlates directory"""
        arr = numpy.array([[raDecPos[0], raDecPos[1]]], numpy.float_)
        ret = self.w.wcs_world2pix(arr, 1)
        ret = [(ret[0][0] - 1.5) * 64., (ret[0][1] - 1.5) * 64.]

        # Add offset to compensate for linear WCS model errors
        if self.gridOffset != None:
            nearestOffset = scipy.interpolate.griddata(self.gridPixelPos, self.gridOffset, [ret], method='nearest')
            ret = [ret[0] + nearestOffset[0][0], ret[1] + nearestOffset[0][1]]

        return ret
