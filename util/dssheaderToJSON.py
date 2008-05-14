#!/usr/bin/python
# Simple script which convert the FITS headers associated to Brian McLean DSS images into simplified JSON files
# Fabien Chereau fchereau@eso.org

import sys
import os
import math
import Image
from astLib import astWCS

levels = ["x64", "x32", "x16", "x8", "x4", "x2", "x1"]
# Define the invalid zones in the plates corners for N and S plates
removeBoxN=[64*300-2000, 1150]
removeBoxS=[460, 604]

def getIntersectPoly(baseFileName, curLevel, i,j):
	"""Return the convex polygons defining the valid area of the tile or None if the poly is fully in the invalid area"""
	scale = 2**(6-curLevel)*300
	if baseFileName[0]=='N':
		box = removeBoxN
		x=float(box[0]-i*scale)/scale*300.
		y=float(box[1]-j*scale)/scale*300.
		
		if x>300. or y<0.:
			# Tile fully valid
			return [[[0,0], [300, 0], [300, 300], [0, 300]]]

		if x<=0. and y>=300.:
			# Tile fully invalid
			return None
				
		if x<=0.:
			assert y>0
			assert y<=300.
			return [[[0,y], [300, y], [300, 300], [0, 300]]]
		if y>=300.:
			assert x>0
			assert x<=300.
			return [[[0,0], [x, 0], [x, 300], [0, 300]]]
		return [[[0,0], [x, 0], [x, 300], [0, 300]],[[x,y], [300, y], [300, 300], [x, 300]]]
	else:
		box = removeBoxS


def writePolys(pl, wcs, f):
	"""Write the polygons defined in pixel"""
	f.write('\t"skyConvexPolygons" : [')
	for idx,poly in enumerate(pl):
		f.write('[')
		for iv,v in enumerate(poly):
			pos = wcs.pix2wcs(v[0]+0.5,v[1]+0.5)
			f.write('[%.8f, %.8f]' % (pos[0], pos[1]))
			if iv!=len(poly)-1:
				f.write(', ')
		f.write(']')
		if idx!=len(pl)-1:
			f.write(', ')
	f.write('],\n')
	
	naxis1 = wcs.header.get('NAXIS1')
	naxis2 = wcs.header.get('NAXIS2')
	f.write('\t"textureCoords" : [')
	for idx,poly in enumerate(pl):
		f.write('[')
		for iv,v in enumerate(poly):
			pos = (float(v[0])/naxis1,float(v[1])/naxis2)
			f.write('[%.8f, %.8f]' % (pos[0], pos[1]))
			if iv!=len(poly)-1:
				f.write(', ')
		f.write(']')
		if idx!=len(pl)-1:
			f.write(', ')
	f.write('],\n')
	

def writeTile(imgName, polys, curLevel, i,j, f, specialx64=None):
	"""Write the tile info in the file f"""
	baseFileName = imgName+"_%.2d_%.2d_" % (i,j) +levels[curLevel]
	wcs=astWCS.WCS(imgName+"/"+levels[curLevel]+"/"+baseFileName+".hhh")
	f.write('{\n')
	if curLevel==0:
		f.write('\t"credits" : "Copyright (C) 2008 Brian McLean, STScI Digitized Sky Survey",\n')
		f.write('\t"infoUrl" : "http://stdatu.stsci.edu/cgi-bin/dss_form",\n')
	naxis1 = wcs.header.get('NAXIS1')
	naxis2 = wcs.header.get('NAXIS2')
	if (specialx64 == None):
		f.write('\t"imageUrl" : "'+levels[curLevel]+"/"+baseFileName+".jpg"+'",\n')
	writePolys(polys, wcs, f)
	pos10 = wcs.pix2wcs(1,0)
	pos01 = wcs.pix2wcs(0,1)
	pos0 = wcs.pix2wcs(0,0)
	f.write('\t"minResolution" : %f' % max(abs(pos10[0]-pos0[0])*math.cos(pos0[1]*math.pi/180.), abs(pos01[1]-pos0[1])))
	if curLevel<len(levels)-1:
		f.write(',\n')
		f.write('\t"subTiles" :\n\t[\n')
		if curLevel%2==0:
			if (specialx64 == None):
				tmpDirName = imgName
			else:
				tmpDirName = imgName+'/'+imgName
			if getIntersectPoly(baseFileName, curLevel+1, 2*i, 2*j)!=None:
				f.write('\t\t"'+tmpDirName+"_%.2d_%.2d_" % (2*i,2*j) +levels[curLevel+1]+'.json",\n')
			if getIntersectPoly(baseFileName, curLevel+1, 2*i+1, 2*j)!=None:
				f.write('\t\t"'+tmpDirName+"_%.2d_%.2d_" % (2*i+1,2*j) +levels[curLevel+1]+'.json",\n')
			if getIntersectPoly(baseFileName, curLevel+1, 2*i+1, 2*j+1)!=None:
				f.write('\t\t"'+tmpDirName+"_%.2d_%.2d_" % (2*i+1,2*j+1) +levels[curLevel+1]+'.json",\n')
			if getIntersectPoly(baseFileName, curLevel+1, 2*i, 2*j+1)!=None:
				f.write('\t\t"'+tmpDirName+"_%.2d_%.2d_" % (2*i,2*j+1) +levels[curLevel+1]+'.json"')
			if (specialx64 == None):
				f.write(',\n')
		else:
			polys2 = getIntersectPoly(baseFileName, curLevel+1, 2*i, 2*j)
			if polys2!=None:
				writeTile(imgName, polys2, curLevel+1, 2*i,2*j, f)
				f.write(",\n")
			polys2 = getIntersectPoly(baseFileName, curLevel+1, 2*i+1, 2*j)
			if polys2!=None:
				writeTile(imgName, polys2, curLevel+1, 2*i+1,2*j, f)
				f.write(",\n")
			polys2 = getIntersectPoly(baseFileName, curLevel+1, 2*i+1, 2*j+1)
			if polys2!=None:
				writeTile(imgName, polys2, curLevel+1, 2*i+1,2*j+1, f)
				f.write(",\n")
			polys2 = getIntersectPoly(baseFileName, curLevel+1, 2*i, 2*j+1)
			if polys2!=None:
				writeTile(imgName, polys2, curLevel+1, 2*i,2*j+1, f)
				f.write(",\n")
		f.seek(-2, os.SEEK_CUR)
		f.write('\n\t]\n')
	else:
		f.write('\n')
	f.write('}')


def generateOne(imgName, outImgName):
	curLevel = 0
	if not os.path.exists(outImgName):
		os.makedirs(outImgName)
	
	# Create a reduced 256x256 version of all the jpeg
	for curLevel in range(0,len(levels)):
		if not os.path.exists(outImgName+"/"+levels[curLevel]):
			os.makedirs(outImgName+"/"+levels[curLevel])
			print "Create directory "+outImgName+"/"+levels[curLevel]
		for i in range (0,2**curLevel):
			for j in range(0,2**curLevel):
				baseFileName = imgName+"_%.2d_%.2d_" % (i,j) +levels[curLevel]
				im = Image.open(imgName+"/"+levels[curLevel]+"/"+baseFileName+".jpg")
				im3 = im.point(lambda t : 2.*t-256.*(t/256.)**1.6)
				im2 = im3.transform((256, 256), Image.EXTENT, (0,0,300,300), Image.BILINEAR)
				im2.save(outImgName+"/"+levels[curLevel]+"/"+baseFileName+".jpg")
	
	# Create all the JSON files
	for curLevel in range(0,len(levels)):
		if curLevel%2==1:
			for i in range (0,2**curLevel):
				for j in range(0,2**curLevel):
					baseFileName = imgName+"_%.2d_%.2d_" % (i,j) +levels[curLevel]
					polys = getIntersectPoly(baseFileName, curLevel, i,j)
					if polys!=None:
						f = open(outImgName+"/"+baseFileName+".json",'w')
						writeTile(imgName, polys, curLevel, i, j, f)
						f.close()	

def createx64(imgName):
	# Create the x64 level JSON
	baseFileName = imgName+"_00_00_x64"
	polys = getIntersectPoly(baseFileName,0,0,0)
	if polys!=None:
		f = open("/home/fab1/Desktop/allDSS.json",'a')
		writeTile(imgName, polys, 0, 0, 0, f, True)
		f.write(',\n')
		f.close()	
		
def main():
	if len(sys.argv) < 3:
		print "Usage: "+sys.argv[0]+" DSSDirName DSSOutDirName"
		exit(0)
	generateOne(sys.argv[1], sys.argv[2])
	command = "scp -r " +outImgName+ " vosw@voint1.hq.eso.org:/work/fabienDSS/" + imgName;
	print command
	system(command)
	
def all():
	tmpDir = "/tmp/tmpPlate"
	os.system("rm -r "+tmpDir)
	
	nRange = range(2,894)
	
	for i in nRange:
		dirName = "N%.3i" % i
		createx64(dirName)
		
	for i in nRange:
		dirName = "N%.3i" % i
		generateOne(dirName, tmpDir)
	
		command = "scp -r " +tmpDir+ " vosw@voint1.hq.eso.org:/work/fabienDSS/" + dirName;
		print command
		os.system(command)
		os.system("rm -r "+tmpDir)
	
	
if __name__ == "__main__":
    all()