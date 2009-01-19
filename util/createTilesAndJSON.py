#!/usr/bin/python
# Simple script which convert a large image + FITS headers into a bunch of multiresolution tiles + JSON description files
# Fabien Chereau fchereau@eso.org

import sys
import os
import Image
import ImageOps
import math
from astLib import astWCS
from skyTile import *
from optparse import OptionParser
from optparse import IndentedHelpFormatter

def sphe2rect3d(lng, lat):
	cosLat = math.cos(lat)
	return [math.cos(lng) * cosLat, math.sin(lng) * cosLat, math.sin(lat)]

def dot(v1, v2):
	return v1[0]*v2[0]+v1[1]*v2[1]+v1[2]*v2[2]

def lengthSquared(v):
    return v[0]*v[0]+v[1]*v[1]+v[2]*v[2]

def angularDist(v1, v2):
	return math.acos(dot(v1,v2)/math.sqrt(lengthSquared(v1)*lengthSquared(v2)));

def createTile(currentLevel, maxLevel, i, j, wcs, im, doImage, tileSize):
	if (currentLevel>=maxLevel):
		return None
	
	# Create the jpg sub tile
	realTileSize = tileSize*2**(maxLevel-currentLevel-1)
	box = ( i*realTileSize, j*realTileSize, min((i+1)*realTileSize, im.size[0]), min((j+1)*realTileSize, im.size[1]) )
	if (box[0]>=im.size[0] or box[1]>=im.size[1]):
		return None
	imgName = 'x%.2i_%.2i_%.2i.jpg' % (2**currentLevel, i, j)
	if doImage:
		region = im.crop(box)
		r2 = region.resize((tileSize, tileSize), Image.ANTIALIAS)
		r2.save(imgName)
	
	# Create the associated tile description
	t = SkyImageTile()
	t.level = currentLevel
	t.i = i
	t.j = j
	t.imageUrl = imgName
	v00 = wcs.pix2wcs(box[0]+0.5,im.size[1]-box[3]+0.5)
	v10 = wcs.pix2wcs(box[2]+0.5,im.size[1]-box[3]+0.5)
	v01 = wcs.pix2wcs(box[0]+0.5,im.size[1]-box[1]+0.5)
	v11 = wcs.pix2wcs(box[2]+0.5,im.size[1]-box[1]+0.5)
	t.skyConvexPolygons = [[ [v00[0],v00[1]], [v10[0],v10[1]], [v11[0],v11[1]], [v01[0],v01[1]] ]]
	t.textureCoords = [[ [0,0], [1,0], [1,1], [0,1] ]]
	t.minResolution = max( angularDist(sphe2rect3d(v10[0]*math.pi/180., v10[1]*math.pi/180.), sphe2rect3d(v00[0]*math.pi/180., v00[1]*math.pi/180.)), angularDist(sphe2rect3d(v01[0]*math.pi/180., v01[1]*math.pi/180.), sphe2rect3d(v00[0]*math.pi/180.,v00[1]*math.pi/180.)) )*180./math.pi/tileSize
	
	# Recursively creates the 4 sub-tiles
	sub = createTile(currentLevel+1, maxLevel, i*2, j*2, wcs, im, doImage, tileSize)
	if sub!=None:
		t.subTiles.append(sub)
	sub = createTile(currentLevel+1, maxLevel, i*2+1, j*2, wcs, im, doImage, tileSize)
	if sub!=None:
		t.subTiles.append(sub)
	sub = createTile(currentLevel+1, maxLevel, i*2+1, j*2+1, wcs, im, doImage, tileSize)
	if sub!=None:
		t.subTiles.append(sub)
	sub = createTile(currentLevel+1, maxLevel, i*2, j*2+1, wcs, im, doImage, tileSize)
	if sub!=None:
		t.subTiles.append(sub)
	return t


def main():
	parser = OptionParser(usage="%prog imageFile [options]", version="0.1", description="This tool generates multi-resolution astronomical images from large image for display by Stellarium/VirGO. The passed image is split in small tiles and the corresponding index JSON files are generated. Everything is output in the current directory.", formatter=IndentedHelpFormatter(max_help_position=33, width=80))
	parser.add_option("-f", "--fitsheader", dest="fitsHeader", help="use the FITS header from FILE to get WCS info", metavar="FILE")
	parser.add_option("-g", "--gzipcompress", dest="gzipCompress", action="store_true", default=False, help="compress the produced JSON index using gzip")
	parser.add_option("-t", "--tilesize", dest="tileSize", type="int", default=256, help="output image tiles size in pixel (default: %default)", metavar="SIZE")
	parser.add_option("-i", "--onlyindex", dest="makeImageTiles", action="store_false", default=True, help="output only the JSON index")
	parser.add_option("-l", "--maxLevPerIndex", dest="maxLevelPerIndex", default=3, type="int", help="put up to MAX levels per index file (default: %default)", metavar="MAX")
	parser.add_option("-b", "--maxBrightness", dest="maxBrightness", default=13., type="float", help="the surface brightness of a white pixel of the image in V mag/arcmin^2 (default: %default)", metavar="MAG")
	(options, args) = parser.parse_args()
	
	headerFile = None
	if len(args) < 1:
		print "Usage: "+os.path.basename(sys.argv[0])+" imageFile [options]"
		exit(0)		
	imgFile = sys.argv[1]
	
	# We now have valid arguments
	
	if options.fitsHeader!=None:
		# Try to read the provided FITS header file to extract the WCS
		wcs = astWCS.WCS(options.fitsHeader)
	else:
		# Else try to generate the WCS from the passed options
		print "WCS options not yet supported, you need to give a fits header!"
		exit(0)
		(nxpix, nypix, ctype1, ctype2, crpix1, crpix2, crval1, crval2, cd, cdelt1, cdelt2, crota, equinox, epoch) = sys.argv[2:]
		worldCoord = astWCS.wcs.wcskinit(nxpix, nypix, ctype1, ctype2, crpix1, crpix2, crval1, crval2, cd, cdelt1, cdelt2, crota, equinox, epoch)
		coordFrameCstr = 'J2000'
		wcs = astWCS.wcs.wcsininit(worldCoord, coordFrameCstr);
		#[nxpix, nypix, ctype1, ctype2, crpix1, crpix2, crval1, crval2, cd, cdelt1, cdelt2, crota, equinox, epoch]"

	
	im = Image.open(imgFile)
	
	nbTileX = (im.size[0]+options.tileSize)//options.tileSize
	nbTileY = (im.size[1]+options.tileSize)//options.tileSize
	nbLevels = int(math.log(max(nbTileX, nbTileY))/math.log(2)+1)
	print "Will tesselate image (",im.size[0],"x",im.size[1],") in", nbTileX,'x',nbTileY,'tiles on', nbLevels, 'levels'
	
	# Create the master level 0 tile, which recursively creates the subtiles
	masterTile = createTile(0, nbLevels, 0, 0, wcs, im, options.makeImageTiles, options.tileSize)
	masterTile.maxBrightness = options.maxBrightness
	masterTile.outputJSON(qCompress=options.gzipCompress, maxLevelPerFile=options.maxLevelPerIndex)
	
if __name__ == "__main__":
    main()
