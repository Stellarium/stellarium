# Copyright (c) 2007, Simon Edwards <simon@simonzone.com>
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
import PyQt4.pyqtconfig

pyqtcfg = PyQt4.pyqtconfig.Configuration()
print("pyqt_version:%06.0x" % pyqtcfg.pyqt_version)
print("pyqt_version_str:%s" % pyqtcfg.pyqt_version_str)
print("pyqt_sip_dir:%s" % pyqtcfg.pyqt_sip_dir)
