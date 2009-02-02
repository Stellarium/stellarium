#!/usr/bin/python
# Copyright (C) 2009 Fabien Chereau
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
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 

# Tool which converts a large image + FITS headers into a bunch of multiresolution tiles + JSON description files
# Fabien Chereau fchereau@eso.org

import sys
import os
import Image
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
	if (currentLevel>maxLevel):
		return None
	
	# Create the jpg sub tile
	realTileSize = tileSize*2**(maxLevel-currentLevel)
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
	# Program options management
	parser = OptionParser(usage="%prog imageFile [options]", version="0.1", description="This tool generates multi-resolution astronomical images from large image for display by Stellarium/VirGO. The passed image is split in small tiles and the corresponding index JSON files are generated. Everything is output in the current directory.", formatter=IndentedHelpFormatter(max_help_position=33, width=80))
	parser.add_option("-f", "--fitsheader", dest="fitsHeader", help="use the FITS header from FILE to get WCS info", metavar="FILE")
	parser.add_option("-g", "--gzipcompress", dest="gzipCompress", action="store_true", default=False, help="compress the produced JSON index using gzip")
	parser.add_option("-t", "--tilesize", dest="tileSize", type="int", default=256, help="output image tiles size in pixel (default: %default)", metavar="SIZE")
	parser.add_option("-i", "--onlyindex", dest="makeImageTiles", action="store_false", default=True, help="output only the JSON index")
	parser.add_option("-l", "--maxLevPerIndex", dest="maxLevelPerIndex", default=3, type="int", help="put up to MAX levels per index file (default: %default)", metavar="MAX")
	parser.add_option("-b", "--maxBrightness", dest="maxBrightness", default=13., type="float", help="the surface brightness of a white pixel of the image in V mag/arcmin^2 (default: %default)", metavar="MAG")
	parser.add_option("-a", "--alphaBlend", dest="alphaBlend", action="store_true", default=False, help="activate alpha blending for this image")
	parser.add_option("--imgInfoShort", dest="imgInfoShort", type="string", help="the short name of the image", metavar="STR")
	parser.add_option("--imgInfoFull", dest="imgInfoFull", type="string", help="the full name of the image", metavar="STR")
	parser.add_option("--imgInfoUrl", dest="imgInfoUrl", type="string", help="the info URL about the image", metavar="STR")
	parser.add_option("--imgCreditsShort", dest="imgCreditsShort", type="string", help="the short name of the image creator", metavar="STR")
	parser.add_option("--imgCreditsFull", dest="imgCreditsFull", type="string", help="the full name of the image creator", metavar="STR")
	parser.add_option("--imgCreditsInfoUrl", dest="imgCreditsInfoUrl", type="string", help="the info URL about the image creator", metavar="STR")
	parser.add_option("--srvCreditsShort", dest="serverCreditsShort", type="string", help="the short name of the hosting server", metavar="STR")
	parser.add_option("--srvCreditsFull", dest="serverCreditsFull", type="string", help="the full name of the hosting server", metavar="STR")
	parser.add_option("--srvCreditsInfoUrl", dest="serverCreditsInfoUrl", type="string", help="the info URL about the hosting server", metavar="STR")
	(options, args) = parser.parse_args()
	
	# Need at least an image file as input
	if len(args) < 1:
		print "Usage: "+os.path.basename(sys.argv[0])+" imageFile [options]"
		exit(0)		
	imgFile = sys.argv[1]
	
	# We now have valid arguments
	if options.fitsHeader!=None:
		# Try to read the provided FITS header file to extract the WCS
		wcs = astWCS.WCS(options.fitsHeader)
	else:
		# Else try to generate the WCS from the xmp informations contained in the file header
		import libxmp
		print "Try to import WCS info from the XMP headers in the image"
		libxmp.XMPFiles.initialize()
		xmpfile = libxmp.XMPFiles()
		xmpfile.open_file(imgFile, libxmp.consts.XMP_OPEN_READ)
		xmp = xmpfile.get_xmp()
		ns = 'http://www.communicatingastronomy.org/avm/1.0/'
		nxpix=int(xmp.get_array_item(ns, 'Spatial.ReferenceDimension', 1).keys()[0])
		nypix=int(xmp.get_array_item(ns, 'Spatial.ReferenceDimension', 2).keys()[0])
		ctype1='RA---TAN'
		ctype2='DEC--TAN'
		crpix1=float(xmp.get_array_item(ns, 'Spatial.ReferencePixel', 1).keys()[0])
		crpix2=float(xmp.get_array_item(ns, 'Spatial.ReferencePixel', 2).keys()[0])
		crval1=float(xmp.get_array_item(ns, 'Spatial.ReferenceValue', 1).keys()[0])
		crval2=float(xmp.get_array_item(ns, 'Spatial.ReferenceValue', 2).keys()[0])
		cdelt1=float(xmp.get_array_item(ns, 'Spatial.Scale', 1).keys()[0])
		cdelt2=float(xmp.get_array_item(ns, 'Spatial.Scale', 2).keys()[0])
		crota=float(xmp.get_property(ns, "Spatial.Rotation"))
		equinox=xmp.get_property(ns, "Spatial.Equinox")
		if equinox=='J2000':
			equinox=2000
		elif equinox=='B1950':
			equinox=1950
		else:
			equinox=float(equinox)
		coordFrameCstr=xmp.get_property(ns, "Spatial.CoordinateFrame")
		
		header = astWCS.pyfits.Header()
		header.update('SIMPLE', 'T')
		header.update('BITPIX', 8)
		header.update('NAXIS', 2)
		header.update('NAXIS1', nxpix)
		header.update('NAXIS2', nypix)
		header.update('CDELT1', cdelt1)
		header.update('CDELT2', cdelt2)
		header.update('CTYPE1', 'RA---TAN')
		header.update('CTYPE2', 'DEC--TAN')
		header.update('CRVAL1', crval1)
		header.update('CRVAL2', crval2)
		header.update('CRPIX1', crpix1)
		header.update('CRPIX2', crpix2)
		header.update('CROTA', crota)
		header.update('EQUINOX', equinox)
		header.update('RADECSYS', coordFrameCstr)
		wcs = astWCS.WCS(header, mode='pyfits')
	
	im = Image.open(imgFile)
	
	nbTileX = (im.size[0]+options.tileSize)//options.tileSize
	nbTileY = (im.size[1]+options.tileSize)//options.tileSize
	maxSize = max(im.size[0], im.size[1])
	nbLevels = 0
	while 2**nbLevels*options.tileSize<maxSize:
		nbLevels+=1
	print "Will tesselate image (",im.size[0],"x",im.size[1],") in", nbTileX,'x',nbTileY,'tiles on', nbLevels+1, 'levels'
	
	# Create the master level 0 tile, which recursively creates the subtiles
	masterTile = createTile(0, nbLevels, 0, 0, wcs, im, options.makeImageTiles, options.tileSize)
	
	# Add some top-level informations
	masterTile.maxBrightness = options.maxBrightness
	masterTile.alphaBlend = options.alphaBlend
	masterTile.imageInfo.short = options.imgInfoShort
	masterTile.imageInfo.full = options.imgInfoFull
	masterTile.imageInfo.infoUrl = options.imgInfoUrl
	masterTile.imageCredits.short = options.imgCreditsShort
	masterTile.imageCredits.full = options.imgCreditsFull
	masterTile.imageCredits.infoUrl = options.imgCreditsInfoUrl
	masterTile.serverCredits.short = options.serverCreditsShort
	masterTile.serverCredits.full = options.serverCreditsFull
	masterTile.serverCredits.infoUrl = options.serverCreditsInfoUrl

	# masterTile contains the whole tree, just need to output it as desired
	masterTile.outputJSON(qCompress=options.gzipCompress, maxLevelPerFile=options.maxLevelPerIndex)
	
if __name__ == "__main__":
    main()
