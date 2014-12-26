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


# -----------------------------------------------------------
# Generate the survey, i.e.
# Cut and re-project toast tiles from color DSS plates
# -----------------------------------------------------------
# python generateFullPlate.py
#
# input : the "preparedPlates" directory created in step 1 must be present
# output: a "results" directory containing the survey ready for display in Stellarium


import Image, ImageFilter, ImageStat, ImageChops, ImageOps, ImageDraw, ImageFont
import os
import json
from subprocess import Popen, PIPE, STDOUT
import sys
import dssUtils

# Test code where I tried to use a low res image from Axel mellinger as reference
# for color harmonization (the image we use for the milky way in Stellarium)
# It's not working well yet, so let commented for now
#mellingerImage = Image.open("mellinger_rgb_rect-curved.jpg")
#mellingerImagePix = mellingerImage.load()

def generatePlate(plateName, maxLevel):
    if not os.path.exists("results"):
        os.system("mkdir results")
    if not os.path.exists("results/%d" % maxLevel):
        os.system("mkdir results/%d" % maxLevel)
    if not os.path.exists("tmp"):
        os.system("mkdir tmp")

    print "-------- " + plateName + " --------"
    if os.path.exists("results/%s.stamp" % (plateName)):
        print "Plate %s already processed" % (plateName)
        return

    assert os.path.exists("preparedPlates")
    assert os.path.exists("preparedPlates/%s/%s.jpg" % (plateName, plateName))
    
    print "Get list of TOAST tiles intersecting the plate"
    
    wcs = dssUtils.DssWcs(plateName)
    
    # And make sure to get rid of the plate edges in the corner..
    #points = dssUtils.getValidRegionForPlate(plateName)
    points = [[0,0], [19200, 0], [19200, 19200], [0, 19200]]
    plateCornersRaDec = [wcs.pixelToRaDec(p) for p in points]
    cmd = './toastForShape %d "' % (maxLevel) + json.dumps(plateCornersRaDec).replace('"', '\\"') + '"'
    print cmd
    p = Popen(cmd, shell=True, stdin=PIPE, stdout=PIPE, stderr=STDOUT, close_fds=True)
    jsonTriangles = p.stdout.read()
    triangles = json.loads(jsonTriangles)

    print "Convert the pixel position of the toast tiles into the existing DSS image"
    allTrianglesRadecPos = []
    for tri in triangles:
        allTrianglesRadecPos.extend(tri['tile'][1:])
    allTrianglesPixelPos = [wcs.raDecToPixel(p) for p in allTrianglesRadecPos]

    # Put all contained triangles (to be cut) in the tiles dict
    tiles = {}
    i=0
    for tri in triangles:
        tile = allTrianglesPixelPos[i*4:i*4+4]
        t = {'XY': [tri['i'], tri['j']], 'tile': tile}
        tiles[json.dumps(t['XY'])]=t
        i=i+1

    tiles = tiles.values()

    print "Cut and save the toast tiles (%d tiles at level %s)." % (len(tiles), maxLevel)

    # Create the sub directories

    os.system("mkdir tmp/%s" % plateName)
    os.system("mkdir tmp/%s/%d" % (plateName, maxLevel))


    print "load full plate image"
    resImg = Image.open("preparedPlates/%s/%s.jpg" % (plateName, plateName))
    #draw = ImageDraw.Draw(resImg)
    #font = ImageFont.truetype("DejaVuSans.ttf", 600)
    #draw.text([9500, 9500], plateName, font=font, fill=(255,255,0))
    
    resImg = ImageOps.flip(resImg)
        
    def raDecToMellinger(raDec):
        return [(-raDec[0]/360.+0.5)*mellingerImage.size[0], (-raDec[1]/180.+0.5)*mellingerImage.size[1]]
    
    # Generate a low res reference image from Mellinger data
    delta = 19200./256.
    referenceImage = Image.new("RGB", (256, 256))
    #pix = referenceImage.load()
    #for i in range(256):
    #    for j in range(256):
    #        raDec = wcs.pixelToRaDec((i*delta+delta/2, j*delta+delta/2))
    #        if raDec[0]>180:
    #             raDec[0]=raDec[0]-360
    #        mellingerPos = raDecToMellinger(raDec)
    #        pix[i, j] = mellingerImagePix[mellingerPos[0], mellingerPos[1]]
    
    # Enhance plate
    resImg = dssUtils.enhancePlate(plateName, resImg, referenceImage)
 
    def isPartial(imageW, imageH, region):
        for i in range(len(region)/2):
            p = [region[i*2], region[i*2+1]]
            if p[0]<0 or p[0]>imageW or p[1]<0 or p[1]>imageH:
                return True
        return False
      
    print "Cut tiles"
    for tile in tiles:
        tt = []
        partial = False
        for p in tile['tile']:
            x=p[0]
            y=p[1]
            tt.append(x)
            tt.append(y)
        tt = tt[6:]+tt[0:6]
        
        partial = isPartial(resImg.size[0], resImg.size[1], tt)
        im = resImg.transform((256,256), Image.QUAD, tuple(tt), Image.BILINEAR)

        xy = tile['XY']
        im.save("tmp/%s/%d/%d_%d%s.jpg" % (plateName, maxLevel, xy[0], xy[1], "-partial" if partial else ""), quality=95)

    print "package and store results"
    os.system("mv tmp/%s/%d/* results/%d/" % (plateName, maxLevel, maxLevel))
    os.system("rm -rf tmp/%s" % plateName)
    os.system("touch results/%s.stamp" % plateName)


if __name__ == '__main__':
  maxLevel = 11

  for plate in dssUtils.getAllPlatesNames():
      generatePlate(plate, maxLevel)

  os.system("python createUpperToastLevels.py")
