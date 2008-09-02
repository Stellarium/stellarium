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

TILE_SIZE=256

def createTile(currentLevel, maxLevel, i, j, wcs, im, doImage):
	if (currentLevel>=maxLevel):
		return None
	
	# Create the jpg sub tile
	realTileSize = TILE_SIZE*2**(maxLevel-currentLevel-1)
	box = ( i*realTileSize, j*realTileSize, min((i+1)*realTileSize, im.size[0]), min((j+1)*realTileSize, im.size[1]) )
	if (box[0]>=im.size[0] or box[1]>=im.size[1]):
		return None
	imgName = 'x%.2i_%.2i_%.2i.jpg' % (2**currentLevel, i, j)
	if doImage:
		region = im.crop(box)
		r2 = region.resize((TILE_SIZE, TILE_SIZE), Image.ANTIALIAS)
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
	t.minResolution = max(abs(v10[0]-v00[0])*math.cos(v00[1]*math.pi/180.), abs(v01[1]-v00[1]))/TILE_SIZE
	
	# Recursively creates the 4 sub-tiles
	sub = createTile(currentLevel+1, maxLevel, i*2, j*2, wcs, im, doImage)
	if sub!=None:
		t.subTiles.append(sub)
	sub = createTile(currentLevel+1, maxLevel, i*2+1, j*2, wcs, im, doImage)
	if sub!=None:
		t.subTiles.append(sub)
	sub = createTile(currentLevel+1, maxLevel, i*2+1, j*2+1, wcs, im, doImage)
	if sub!=None:
		t.subTiles.append(sub)
	sub = createTile(currentLevel+1, maxLevel, i*2, j*2+1, wcs, im, doImage)
	if sub!=None:
		t.subTiles.append(sub)
	return t


def main():
	headerFile = None
	if len(sys.argv) == 3:
		headerFile = sys.argv[2]
	elif len(sys.argv) != 15:
		print "Usage: "+sys.argv[0]+" imageFile [FITSHeader] [nxpix, nypix, ctype1, ctype2, crpix1, crpix2, crval1, crval2, cd, cdelt1, cdelt2, crota, equinox, epoch]"
		exit(0)

	imgFile = sys.argv[1]
	
	# Try to read the header file provided to extract the WCS
	if headerFile != None:
		wcs = astWCS.WCS(headerFile)
	else:
		# Or try to generate the WCS by hand
		(nxpix, nypix, ctype1, ctype2, crpix1, crpix2, crval1, crval2, cd, cdelt1, cdelt2, crota, equinox, epoch) = sys.argv[2:]
		worldCoord = astWCS.wcs.wcskinit(nxpix, nypix, ctype1, ctype2, crpix1, crpix2, crval1, crval2, cd, cdelt1, cdelt2, crota, equinox, epoch)
		coordFrameCstr = 'J2000'
		wcs = astWCS.wcs.wcsininit(worldCoord, coordFrameCstr);
	
	im = Image.open(imgFile)
	
	nbTileX = (im.size[0]+TILE_SIZE)//TILE_SIZE
	nbTileY = (im.size[1]+TILE_SIZE)//TILE_SIZE
	nbLevels = int(math.log(max(nbTileX, nbTileY))/math.log(2)+1)
	print "Will tesselate image (",im.size[0],"x",im.size[1],") in", nbTileX,'x',nbTileY,'tiles on', nbLevels, 'levels'
	
	# Create the master level 0 tile, which recursively creates the subtiles
	masterTile = createTile(0, nbLevels, 0, 0, wcs, im, False)
	masterTile.maxBrightness = 13.
	masterTile.credits = "ESO"
	masterTile.infoUrl = ""
	
	masterTile.outputJSON(qCompress=False, maxLevelPerFile=3)
	
if __name__ == "__main__":
    main()