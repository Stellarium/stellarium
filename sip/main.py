#!/usr/bin/python

import sys
from PyQt4 import QtGui
from PyQt4 import QtCore

from PyStellarium import *
from ui_mainGui import Ui_Form

app = QtGui.QApplication(sys.argv)
mainWin = StelMainWindow()

ui = Ui_Form()
ui.setupUi(mainWin)
mainWin.init()

# The actions need to be added to the main form to be effective
for a in mainWin.children():
	if type(a) is QtGui.QAction:
		mainWin.addAction(a);

modMgr = StelApp.getInstance().getModuleMgr()

module = modMgr.getModule("ConstellationMgr")
QtCore.QObject.connect(ui.actionShow_Constellation_Lines,QtCore.SIGNAL("toggled(bool)"),module, QtCore.SLOT("setFlagLines(bool)"))
QtCore.QObject.connect(ui.actionShow_Constellation_Labels,QtCore.SIGNAL("toggled(bool)"),module, QtCore.SLOT("setFlagNames(bool)"))
QtCore.QObject.connect(ui.actionShow_Constellation_Art,QtCore.SIGNAL("toggled(bool)"),module, QtCore.SLOT("setFlagArt(bool)"))


class PyStelModule(StelModule):
	def init():
		print "init"
	def draw(core):
		print "draw"
		return 0.
	def update(deltaTime):
		print deltaTime

testModule = PyStelModule()
modMgr.registerModule(testModule)

sys.exit(app.exec_())