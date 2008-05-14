#!/usr/bin/python
# Fabien Chereau fchereau@eso.org

import sys
import os
import math
import qt

class SkyImageTile:
	def __init__(self):
		self.subTiles = []
		self.credits = None
		self.infoUrl = None
		self.imageUrl = None
		self.luminance = None
		return
	
	def writePolys(self, pl, f):
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
		
	def outputJSON(self, prefix='', qCompress=False, maxLevelPerFile=10):
		"""Output the tiles tree in the JSON format"""
		fName = prefix+"x%.2d_%.2d_%.2d.json" % (2**self.level, self.i, self.j)
		
		# Actually write the file with maxLevelPerFile level
		f = open(fName, 'w')
		self.__subOutJSON(prefix, qCompress, maxLevelPerFile, f, 0)
		f.close()
		
		if (qCompress):
			ff = qt.QFile(fName)
			ff.open(qt.IO_ReadOnly)
			data = qt.qCompress(ff.readAll())
			fout = open(fName+".qZ", 'w')
			fout.write(data.data())
			fout.close()
			os.remove(fName)

	def __subOutJSON(self, prefix, qCompress, maxLevelPerFile, f, curLev):
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
		f.write(levTab+'\t"skyConvexPolygons" : ')
		self.writePolys(self.skyConvexPolygons, f)
		f.write(',\n')
		f.write(levTab+'\t"textureCoords" : ')
		self.writePolys(self.textureCoords, f)
		f.write(',\n')
		if self.luminance:
			f.write(levTab+'\t"luminance" : %f,\n' % 1.0)
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
				st.__subOutJSON(prefix, qCompress, maxLevelPerFile, f, curLev+1)
				f.write(',\n')
		else:
			# Write the tiles in a new file
			for st in self.subTiles:
				st.outputJSON(prefix, qCompress, maxLevelPerFile)
				f.write(levTab+'\t\t"'+prefix+"x%.2d_%.2d_%.2d.json" % (2**st.level, st.i, st.j))
				if qCompress:
					f.write(".qZ")
				f.write('",\n')
		f.seek(-2, os.SEEK_CUR)
		f.write('\n'+levTab+'\t]\n')
		f.write(levTab+'}')
