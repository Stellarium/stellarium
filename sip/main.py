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
	def __init__(self, paintDevice):
		StelModule.__init__(self)
		self.setObjectName("PyStelModule")
		self.paintDevice = paintDevice
		self.scene = QtGui.QGraphicsScene()
		self.scene.addText("Hello, world! Ca va?")
		for i in range(1,100):
			self.scene.addEllipse(i,i,i,i)
		self.painter = QtGui.QPainter()
	def getCallOrder(self, actionName):
		if (actionName==StelModule.ACTION_DRAW):
			return 10000.
		return 0.
	def init(self):
		pass
	def draw(self, core):
		self.painter.begin(self.paintDevice)
		self.painter.setRenderHint(QtGui.QPainter.Antialiasing)
		self.scene.render(self.painter, QtCore.QRectF(100,100,100,100))
		self.painter.end()
		return 0.
	def update(self, deltaTime):
		pass

testModule = PyStelModule(mainWin.getOpenGLWin())
modMgr.registerModule(testModule, True)

sys.exit(app.exec_())