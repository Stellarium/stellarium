#!/usr/bin/python
# Fabien Chereau fchereau@eso.org

import sys
import os
import math
import gzip

def writePolys(pl, f):
	f.write('[')
	for idx,poly in enumerate(pl):
		f.write('[')
		for iv,v in enumerate(poly):
			f.write('[%.8f, %.8f]' % (v[0], v[1]))
			if iv!=len(poly)-1:
				f.write(', ')
		f.write(']')
		if idx!=len(pl)-1:
			f.write(', ')
	f.write(']')
		
class SkyImageTile:
	def __init__(self):
		self.subTiles = []
		self.credits = None
		self.infoUrl = None
		self.imageUrl = None
		self.maxBrightness = None
		return
	
	def outputJSON(self, prefix='', qCompress=False, maxLevelPerFile=10, outDir=''):
		"""Output the tiles tree in the JSON format"""
		fName = outDir+prefix+"x%.2d_%.2d_%.2d.json" % (2**self.level, self.i, self.j)
		
		# Actually write the file with maxLevelPerFile level
		f = open(fName, 'w')
		self.__subOutJSON(prefix, qCompress, maxLevelPerFile, f, 0, outDir)
		f.close()
		
		if (qCompress):
			ff = open(fName)
			fout = gzip.GzipFile(fName+".gz", 'w')
			fout.write(ff.read())
			fout.close()
			os.remove(fName)

	def __subOutJSON(self, prefix, qCompress, maxLevelPerFile, f, curLev, outDir):
		levTab = ""
		for i in range(0,curLev):
			levTab += '\t'
		
		f.write(levTab+'{\n')
		if self.credits:
			f.write(levTab+'\t"credits" : "'+self.credits+'",\n')
		if self.infoUrl:
			f.write(levTab+'\t"infoUrl" : "'+self.infoUrl+'",\n')
		if self.imageUrl:
			f.write(levTab+'\t"imageUrl" : "'+self.imageUrl+'",\n')
		f.write(levTab+'\t"worldCoords" : ')
		writePolys(self.skyConvexPolygons, f)
		f.write(',\n')
		f.write(levTab+'\t"textureCoords" : ')
		writePolys(self.textureCoords, f)
		f.write(',\n')
		if self.maxBrightness:
			f.write(levTab+'\t"maxBrightness" : %f,\n' % self.maxBrightness)
		f.write(levTab+'\t"minResolution" : %f' % self.minResolution)
		if len(self.subTiles)==0:
			f.write('\n'+levTab+'}')
			return
		f.write(',\n')
		f.write(levTab+'\t"subTiles" : [\n')
		
		if curLev+1<maxLevelPerFile:
			# Write the tiles in the same file
			for st in self.subTiles:
				assert isinstance(st, SkyImageTile)
				st.__subOutJSON(prefix, qCompress, maxLevelPerFile, f, curLev+1, outDir)
				f.write(',\n')
		else:
			# Write the tiles in a new file
			for st in self.subTiles:
				st.outputJSON(prefix, qCompress, maxLevelPerFile, outDir)
				f.write(levTab+'\t\t{"$ref": "'+prefix+"x%.2d_%.2d_%.2d.json" % (2**st.level, st.i, st.j))
				if qCompress:
					f.write(".gz")
				f.write('"},\n')
		f.seek(-2, os.SEEK_CUR)
		f.write('\n'+levTab+'\t]\n')
		f.write(levTab+'}')
