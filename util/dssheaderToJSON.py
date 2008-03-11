#!/usr/bin/python
# Simple script which convert the FITS headers associated to Brian McLean DSS images into simplified JSON files
# Fabien Chereau fchereau@eso.org

import sys
from astLib import astWCS

if len(sys.argv) < 2:
	print "Usage: "+sys.argv[0]+" DSSDirName"
	exit(0)
imgName = sys.argv[1]

levels = ["x64", "x32", "x16", "x8", "x4", "x2", "x1"]
curLevel = 0
for lstr in levels:
	for i in range (0,2**curLevel):
		for j in range(0,2**curLevel):
			baseFileName = imgName+"_%.2d_%.2d_" % (i,j) +lstr
			wcs=astWCS.WCS(lstr+"/"+baseFileName+".hhh")
			f = open(baseFileName+".json",'w')
			f.write('{\n')
			if curLevel==0:
				f.write('\t"credits" : "Copyright (C) 2008 Brian McLean, STScI Digitized Sky Survey",\n')
				f.write('\t"infoUrl" : "http://stdatu.stsci.edu/cgi-bin/dss_form",\n')
			f.write('\t"imageUrl" : "'+lstr+"/"+baseFileName+".jpg"+'",\n')
			f.write('\t"skyConvexPolygons" : [[')
			naxis1 = wcs.header.get('NAXIS1')
			naxis2 = wcs.header.get('NAXIS2')
			pos = wcs.pix2wcs(0.5,0.5)
			f.write('[%.8f, %.8f], ' % (pos[0], pos[1]))
			pos = wcs.pix2wcs(naxis1+0.5,0.5)
			f.write('[%.8f, %.8f], ' % (pos[0], pos[1]))
			pos = wcs.pix2wcs(naxis1+0.5,naxis2+0.5)
			f.write('[%.8f, %.8f], ' % (pos[0], pos[1]))
			pos = wcs.pix2wcs(0.5,naxis2+0.5)
			f.write('[%.8f, %.8f]' % (pos[0], pos[1]))
			f.write(']],\n')
			f.write('\t"textureCoords" : [[[0,0], [1,0], [1,1], [0,1]]],\n')
			pos10 = wcs.pix2wcs(1,0)
			pos01 = wcs.pix2wcs(0,1)
			pos0 = wcs.pix2wcs(0,0)
			f.write('\t"minResolution" : %f' % max(abs(pos10[0]-pos0[0]), abs(pos01[1]-pos0[1])))
			if curLevel<len(levels)-1:
				f.write(',\n')
				f.write('\t"subTiles" :\n\t[\n')
				f.write('\t\t"'+imgName+"_%.2d_%.2d_" % (2*i,2*j) +levels[curLevel+1]+'.json",\n')
				f.write('\t\t"'+imgName+"_%.2d_%.2d_" % (2*i+1,2*j) +levels[curLevel+1]+'.json",\n')
				f.write('\t\t"'+imgName+"_%.2d_%.2d_" % (2*i+1,2*j+1) +levels[curLevel+1]+'.json",\n')
				f.write('\t\t"'+imgName+"_%.2d_%.2d_" % (2*i,2*j+1) +levels[curLevel+1]+'.json"\n')
				f.write('\t]\n')
			else:
				f.write('\n')
			f.write('}')
			f.close()
	curLevel+=1