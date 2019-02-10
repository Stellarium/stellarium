#!/usr/bin/env python

# This utility is used to correctly populate the Stellarium.app bundle, once 'make install' has been run.  The 'make mac_app'
# target causes this to run.
#
# It copies over into the bundle all direct and inderect frameworks and plugins, and them updates the ID and load paths
# using install_name_tool. It copies over all resources needed. It then updates the stellarium binary itself (the load paths).
#
# Copyright (C) 2013 Timothy Reaves
# Copyright (C) 2014 Alexander Wolf

import os
import shutil
import sys
from subprocess import Popen
from subprocess import PIPE

installDirectory = None
qtFrameworksDirectory = None
qtPluginsDirectory = None
sourceDirectory = None


def processResources():
	'''
	Copy over all required resources.
	'''
	# rename resources folder
	resourcesDirectory = os.path.join(installDirectory, 'Resources')
	oldResourcesDirectory = os.path.join(resourcesDirectory, 'stellarium')
	os.rename(os.path.join(installDirectory, 'share'), resourcesDirectory)
	# move resources into proper place
	for directory in os.listdir(oldResourcesDirectory):
		os.rename(os.path.join(oldResourcesDirectory, directory), os.path.join(resourcesDirectory, directory))
	# remvoe the unneeded directoryies
	shutil.rmtree(os.path.join(resourcesDirectory, 'man'))
	shutil.rmtree(os.path.join(resourcesDirectory, 'stellarium'))
	# copy remaining resources
	shutil.copy(os.path.join(sourceDirectory, 'data/Icon.icns'), resourcesDirectory)
	shutil.copy(os.path.join(sourceDirectory, 'util/qt.conf'), resourcesDirectory)
	shutil.copy(os.path.join(sourceDirectory, 'data/PkgInfo'), installDirectory)
	shutil.copy(os.path.join(sourceDirectory, 'data/Info.plist'), installDirectory)

def getListOfLinkedQtFrameworksForFile(file):
	'''
	A utility method used to create a list of Qt frameworks linked into the argument.
	'''
	args = ['otool', '-L', file]
	frameworks = []
	process = Popen(args, stdout=PIPE, stderr=PIPE)
	output, oerr = process.communicate()
	for line in output.split('\n'):
		line = line.strip()
		if line.find('Qt') is not -1:
			frameworks.append(line[:line.find(' ')])
	return frameworks

def copyFrameworkToApp(framework):
	'''
	Copy the Qt framework into the bundle.
	
	NOTE: OS X 10.11 changes things, and the framework now has @rpath, not the absolute path.
	'''
	# print('%s' % framework)
	if '@rpath' in framework:
		frameworkRoot = os.path.split(os.path.split(os.path.split(framework)[0])[0])[0]
		beginPosition = framework.index('/') + 1
		endPosition = framework.index('/', beginPosition)
		frameworkName = framework[beginPosition:endPosition]
		# print('====> %s : %s' % (frameworkName, qtFrameworksDirectory))
	else:
		frameworkRoot = os.path.split(os.path.split(os.path.split(framework)[0])[0])[0]
		frameworkName = os.path.split(frameworkRoot)[1]
	
	if frameworkName != 'Qt':
		target = os.path.join(installDirectory, 'Frameworks', frameworkName)
		if not os.path.exists(target):
			shutil.copytree(os.path.join(qtFrameworksDirectory, frameworkName), target, symlinks=True, ignore=shutil.ignore_patterns('*debug*', 'Headers', '*.prl'))
	
def updateName(file):
	'''
	file is expected to be the path as output from tool
	'''
	base = file[file.find(file[file.rfind('/'):]) + 1:]
	args = ['install_name_tool', '-id', '@executable_path/../Frameworks/' + base, os.path.join(installDirectory, 'Frameworks', base)]
	# print(args)
	process = Popen(args, stdout=PIPE, stderr=PIPE)
	output, oerr = process.communicate()
	if process.returncode != 0:
		print('Error updating name.\n%s\n%s' % (output, oerr))

def updateLibraryPath(file, where):
	'''
	file is expected to be the path as output from otool
	'where' is the path relative to the installDirectory
	'''
	base = ''
	if '@rpath' in file:
		file = file[file.find('/') + 1:]
		file = os.path.join(installDirectory, 'Frameworks', file)
	if file.endswith('dylib') or file == 'stellarium':
		base = file
		file = os.path.join(installDirectory, where, file)
	else:
		base = file[file.find(file[file.rfind('/'):]) + 1:]

	args = ['install_name_tool', '-change', '', '', os.path.join(installDirectory, where, base )]
	for framework in getListOfLinkedQtFrameworksForFile(file):
		# otool adds self, so ignore it
		if not framework == file:
			args[2] = framework
			args[3] = '@executable_path/../Frameworks' + framework[framework.find(framework[framework.rfind('/'):]):]
			#print(args)
			process = Popen(args, stdout=PIPE, stderr=PIPE)
			output, oerr = process.communicate()
			if process.returncode != 0:
				print('Error updating name.\n%s\n%s' % (output, oerr))

def processFrameworks():
	'''
	Copies over and process all linked Qt frameworks.
	'''
	frameworkDirectory = os.path.join(installDirectory, 'Frameworks')
	os.mkdir(frameworkDirectory)
	allFramework = []
	# First, copy over the stellarium dependencies
	frameworks = getListOfLinkedQtFrameworksForFile(os.path.join(installDirectory, 'MacOS/stellarium'))
	# QtMultimedia?
	if 'QtMultimedia' in ' '.join(frameworks):
		frameworks.append(frameworks[-1].replace('QtMultimedia','QtMultimediaWidgets'))
		# QtOpenGL is required by QtMultimediaWidgets
		frameworks.append(frameworks[-1].replace('QtCore','QtOpenGL'))
	for framework in frameworks:
		copyFrameworkToApp(framework)
		allFramework.append(framework)
	# Now copy over any of the framework's dependencies themselves
	for framework in frameworks:
		for dependentFramework in getListOfLinkedQtFrameworksForFile(framework):
			copyFrameworkToApp(dependentFramework)
			allFramework.append(dependentFramework)
	# for framework in set(allFramework):
		updateName(framework)
		print('Processing %s' % framework)
		updateLibraryPath(framework, 'Frameworks')

def copyPluginDirectory(pluginDirectoryName):
	'''
	Utility function to be called by processPlugins().
	pluginDirectoryName is the name of a plugins subdirectory to copy over.
	'''
	pluginsDirectory = os.path.join(installDirectory, 'plugins')

	toDir = os.path.join(pluginsDirectory, pluginDirectoryName)
	os.mkdir(toDir)
	fromDir = os.path.join(qtPluginsDirectory, pluginDirectoryName)
	for plugin in os.listdir(fromDir):
		# there may be debug versions installed; if so, ignore them
		if (plugin.find('_debug') is -1) and (plugin.find('.dSYM') is -1):
			shutil.copy(os.path.join(fromDir, plugin), toDir)
	# Update all paths
	for plugin in os.listdir(toDir):
		updateLibraryPath(plugin, 'plugins/' + pluginDirectoryName)

def processPlugins():
	'''
	Copies over and process all Qt plugins.
	'''
	#Setup the plugin directory
	pluginsDirectory = os.path.join(installDirectory, 'plugins')
	os.mkdir(pluginsDirectory)

	# copy over image plugins
	copyPluginDirectory('imageformats')
	copyPluginDirectory('iconengines')
	# check multimedia support
	if 'QtMultimedia' in ' '.join(getListOfLinkedQtFrameworksForFile(os.path.join(installDirectory, 'MacOS/stellarium'))):
		copyPluginDirectory('mediaservice')
		copyPluginDirectory('audio')

	# copy over the Cocoa platform plugin; we do  single one here, as we do not want every platform
	os.mkdir(os.path.join(pluginsDirectory, 'platforms'))
	shutil.copy(os.path.join(qtPluginsDirectory, 'platforms', 'libqcocoa.dylib'), os.path.join(pluginsDirectory, 'platforms'))
	for framework in getListOfLinkedQtFrameworksForFile(os.path.join(pluginsDirectory, 'platforms', 'libqcocoa.dylib')):
		copyFrameworkToApp(framework)
		updateLibraryPath(framework, 'Frameworks')
	# Always update paths after copying...
	updateLibraryPath('libqcocoa.dylib', 'plugins/platforms')

def processBin():
	'''
	Moves the binary to the correct location
	'''
	os.rename(os.path.join(installDirectory, 'bin'), os.path.join(installDirectory, 'MacOS'))

def processDMG():
	'''
	Create a DMG bundle
	'''
	dmgDir = os.path.join(installDirectory, '../../Stellarium')
	os.mkdir(dmgDir)
	args = ['cp', '-r', os.path.join(installDirectory, '../../Stellarium.app'), dmgDir]
	process = Popen(args, stdout=PIPE, stderr=PIPE)
	output, oerr = process.communicate()
	if process.returncode != 0:
		print('Error copying application.\n%s\n%s' % (output, oerr))
	args = ['cd', os.path.join(dmgDir, '../')]
	process = Popen(args, stdout=PIPE, stderr=PIPE)
	output, oerr = process.communicate()
	if process.returncode != 0:
		print('Error change directory.\n%s\n%s' % (output, oerr))
	args = ['hdiutil', 'create', '-format', 'UDZO', '-srcfolder', dmgDir, os.path.join(dmgDir, '../Stellarium.dmg')]
	process = Popen(args, stdout=PIPE, stderr=PIPE)
	output, oerr = process.communicate()
	if process.returncode != 0:
		print('Error create a DMG bundle.\n%s\n%s' % (output, oerr))
	args = ['rm', '-rf', dmgDir]
	process = Popen(args, stdout=PIPE, stderr=PIPE)
	output, oerr = process.communicate()
	if process.returncode != 0:
		print('Error deleting temporary directory.\n%s\n%s' % (output, oerr))
	


def main():
	'''
	main expects three arguments:
	'''
	global installDirectory, sourceDirectory, qtFrameworksDirectory, qtPluginsDirectory
	if len(sys.argv) < 4:
		print("usage: mac_app.py ${CMAKE_INSTALL_PREFIX} ${PROJECT_SOURCE_DIR} ${CMAKE_BUILD_TYPE} ${Qt5Core_INCLUDE_DIRS}")
		print(sys.argv)
		return
	installDirectory = sys.argv[1]
	sourceDirectory = sys.argv[2]
	buildType = sys.argv[3] # ignored for now
	qtFrameworksDirectory = sys.argv[4]
	
	# qtDirectory will actually be an include dir; we need to convert to the directory where
	# the actual framework is. Something like /Users/treaves/Qt5.1.1/5.1.1/clang_64/lib/QtCore.framework
	# needs to end up as /Users/bv6679/Qt5.1.1/5.1.1/clang_64/lib/
	if qtFrameworksDirectory.endswith('framework'):
		qtFrameworksDirectory = os.path.dirname(qtFrameworksDirectory)
	else:
		print('The QtCore parameter was expected to be a framework.  Found:' + qtFrameworksDirectory)
	#now, find the plugins directory
	qtPluginsDirectory = os.path.normpath(qtFrameworksDirectory + '/../plugins')
	if not os.path.exists(qtPluginsDirectory):
		print('Could not find plugins directory.')
		return
	qmlDirectory = os.path.normpath(qtFrameworksDirectory + '/../imports/Qt/labs')
	
	processBin()
	processResources()
	processFrameworks()
	processPlugins()
	# update application lib's locations; we need to do this here, as the above rely
	# on the binary with the 'original' paths.
	updateLibraryPath('stellarium', 'MacOS')
	# create a DMG bundle
	# processDMG()
	

if __name__ == '__main__':
	main()
