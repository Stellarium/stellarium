#!/usr/bin/python
#
# Simple script which convert the FITS headers associated to Brian McLean DSS images into simplified JSON files
# Fabien Chereau fchereau@eso.org
#

import math
import os
import sys

import Image
from astLib import astWCS

import skyTile

levels = ["x64", "x32", "x16", "x8", "x4", "x2", "x1"]
# Define the invalid zones in the plates corners for N and S plates
removeBoxN = [64 * 300 - 2000, 1199]
removeBoxS = [480, 624]


def getIntersectPoly(baseFileName, curLevel, i, j):
    """Return the convex polygons in pixel space defining the valid area of the tile or None if the poly is fully in the invalid area"""
    scale = 2 ** (6 - curLevel) * 300
    if baseFileName[0] == 'N':
        box = removeBoxN
        x = float(box[0] - i * scale) / scale * 300.
        y = float(box[1] - j * scale) / scale * 300.
        # x,y is the position of the box top left corner in pixel wrt lower left corner of current tile
        if x > 300. or y <= 0.:
            # Tile fully valid
            return [[[0, 0], [300, 0], [300, 300], [0, 300]]]

        if x <= 0. and y >= 300.:
            # Tile fully invalid
            return None

        if x <= 0.:
            # assert y > 0 # (always true, tested above)
            assert y <= 300.
            return [[[0, y], [300, y], [300, 300], [0, 300]]]
        if y >= 300.:
            assert x > 0
            # assert x <= 300. # (always true, tested above)
            return [[[0, 0], [x, 0], [x, 300], [0, 300]]]
        return [[[0, 0], [x, 0], [x, 300], [0, 300]], [[x, y], [300, y], [300, 300], [x, 300]]]
    else:
        box = removeBoxS
        x = float(i * scale - box[0]) / scale * 300.
        y = float(box[1] - j * scale) / scale * 300.
        # x,y is the position of the box top right corner in pixel wrt lower left corner of current tile
        if x > 0. or y <= 0.:
            # Tile fully valid
            return [[[0, 0], [300, 0], [300, 300], [0, 300]]]

        if x <= -300. and y >= 300.:
            # Tile fully invalid
            return None

        if x <= -300.:
            # assert y > 0 # (always true, tested above)
            assert y <= 300.
            return [[[0, y], [300, y], [300, 300], [0, 300]]]
        if y >= 300.:
            # assert x <= 0 # (always true, tested above)
            assert x > -300.
            return [[[-x, 0], [300, 0], [300, 300], [-x, 300]]]
        return [[[-x, 0], [300, 0], [300, 300], [-x, 300]], [[0, y], [-x, y], [-x, 300], [0, 300]]]


def createTile(currentLevel, maxLevel, i, j, outDirectory, plateName, special=False):
    # Create the associated tile description
    t = skyTile.SkyImageTile()
    t.level = currentLevel
    t.i = i
    t.j = j
    t.imageUrl = "x%.2d/" % (2 ** currentLevel) + "x%.2d_%.2d_%.2d.jpg" % (2 ** currentLevel, i, j)
    if currentLevel == 0:
        t.credits = "Copyright (C) 2008, STScI Digitized Sky Survey"
        t.infoUrl = "http://stdatu.stsci.edu/cgi-bin/dss_form"
    # t.maxBrightness = 10

    # Create the matching sky polygons, return if there is no relevant polygons
    if special is True:
        pl = [[[0, 0], [300, 0], [300, 300], [0, 300]]]
    else:
        pl = getIntersectPoly(plateName, currentLevel, i, j)
    if pl is None or not pl:
        return None

    # Get the WCS from the input FITS header file for the tile
    wcs = astWCS.WCS(plateName + "/" + levels[currentLevel] + "/" + plateName + "_%.2d_%.2d_" % (i, j) + levels[
        currentLevel] + ".hhh")
    naxis1 = wcs.header.get('NAXIS1')
    naxis2 = wcs.header.get('NAXIS2')

    t.skyConvexPolygons = []
    for idx, poly in enumerate(pl):
        p = [wcs.pix2wcs(v[0] + 0.5, v[1] + 0.5) for iv, v in enumerate(poly)]
        t.skyConvexPolygons.append(p)

    t.textureCoords = []
    for idx, poly in enumerate(pl):
        p = [(float(v[0]) / naxis1, float(v[1]) / naxis2) for iv, v in enumerate(poly)]
        t.textureCoords.append(p)

    v10 = wcs.pix2wcs(1, 0)
    v01 = wcs.pix2wcs(0, 1)
    v00 = wcs.pix2wcs(0, 0)
    t.minResolution = max(abs(v10[0] - v00[0]) * math.cos(v00[1] * math.pi / 180.), abs(v01[1] - v00[1]))

    if (currentLevel >= maxLevel):
        return t

    # Recursively creates the 4 sub-tiles
    sub = createTile(currentLevel + 1, maxLevel, i * 2, j * 2, outDirectory, plateName)
    if sub != None:
        t.subTiles.append(sub)
    sub = createTile(currentLevel + 1, maxLevel, i * 2 + 1, j * 2, outDirectory, plateName)
    if sub != None:
        t.subTiles.append(sub)
    sub = createTile(currentLevel + 1, maxLevel, i * 2 + 1, j * 2 + 1, outDirectory, plateName)
    if sub != None:
        t.subTiles.append(sub)
    sub = createTile(currentLevel + 1, maxLevel, i * 2, j * 2 + 1, outDirectory, plateName)
    if sub != None:
        t.subTiles.append(sub)
    return t


def generateJpgTiles(inDirectory, outDirectory):
    # Create a reduced 256x256 version of all the jpeg
    for curLevel in range(0, len(levels)):
        fullOutDir = outDirectory + "/x%.2d" % (2 ** curLevel)
        if not os.path.exists(fullOutDir):
            os.makedirs(fullOutDir)
            print "Create directory " + fullOutDir
        for i in range(0, 2 ** curLevel):
            for j in range(0, 2 ** curLevel):
                baseFileName = "x%.2d_%.2d_%.2d" % (2 ** curLevel, i, j)
                im = Image.open(
                    inDirectory + "/" + levels[curLevel] + "/" + inDirectory + '_' + "%.2d_%.2d_" % (i, j) + levels[
                        curLevel] + ".jpg")
                # Enhance darker part of the image
                im3 = im.point(lambda t: 2. * t - 256. * (t / 256.) ** 1.6)
                im2 = im3.transform((256, 256), Image.EXTENT, (0, 0, 300, 300), Image.BILINEAR)
                im2.save(fullOutDir + '/' + baseFileName + ".jpg")


def plateRange():
    if len(sys.argv) != 4:
        print "Usage: " + sys.argv[0] + " prefix startPlate stopPlate "
        exit(-1)
    prefix = sys.argv[1]
    outDir = "/tmp/tmpPlate"
    nRange = range(int(sys.argv[2]), int(sys.argv[3]))

    for i in nRange:
        if os.path.exists(outDir):
            os.system("rm -r " + outDir)
        os.makedirs(outDir)

        plateName = prefix + "%.3i" % i
        generateJpgTiles(plateName, outDir)

        # Create all the JSON files
        masterTile = createTile(0, 6, 0, 0, outDir, plateName)
        masterTile.outputJSON(qCompress=True, maxLevelPerFile=2, outDir=outDir + '/')

        command = "cd /tmp && mv tmpPlate " + plateName + " && tar -cf " + plateName + ".tar " + plateName + " && rm -rf " + plateName
        print command
        os.system(command)
        command = "cd /tmp && scp " + plateName + ".tar vosw@voint1.hq.eso.org:/work/fabienDSS2/" + plateName + ".tar"
        print command
        os.system(command)
        command = "rm /tmp/" + plateName + ".tar"
        print command
        os.system(command)


def mainHeader():
    # Generate the top level file containing pointers on all
    outDir = "/tmp/tmpPlate"
    with open('/tmp/allDSS.json', 'w') as f:
        f.write('{\n')
        f.write('"minResolution" : 0.1,\n')
        f.write('"maxBrightness" : 13,\n')
        f.write('"subTiles" : \n[\n')

        for prefix in ['N', 'S']:
            if prefix == 'N':
                nRange = range(2, 898)
            if prefix == 'S':
                nRange = range(1, 894)
            for i in nRange:
                plateName = prefix + "%.3i" % i
                ti = createTile(0, 0, 0, 0, outDir, plateName, True)
                assert ti != None
                f.write('\t{\n')
                f.write('\t\t"minResolution" : %.8f,\n' % ti.minResolution)
                f.write('\t\t"worldCoords" : ')
                skyTile.writePolys(ti.skyConvexPolygons, f)
                f.write(',\n')
                f.write('\t\t"subTiles" : ["' + plateName + "/x01_00_00.json.qZ" + '"]\n')
                f.write('\t},\n')

        f.seek(-2, os.SEEK_CUR)
        f.write('\n]}\n')


if __name__ == "__main__":
    import psyco

    psyco.full()
    plateRange()
