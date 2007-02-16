bin_PROGRAMS = stellarium
stellarium_SOURCES = Projector.cpp Projector.hpp \
	Mapping.hpp Mapping.cpp MappingClasses.hpp MappingClasses.cpp \
	SphericMirrorCalculator.cpp SphericMirrorCalculator.hpp \
	ViewportDistorter.cpp ViewportDistorter.hpp Navigator.cpp Navigator.hpp Observer.cpp \
	Observer.hpp grid.cpp grid.h GeodesicGrid.cpp GeodesicGrid.hpp \
	StelObjectBase.hpp StelObject.cpp StelObject.hpp StelObjectType.hpp \
	StarMgr.cpp StarMgr.hpp Nebula.cpp Nebula.hpp NebulaMgr.cpp NebulaMgr.hpp \
	Constellation.cpp Constellation.hpp ConstellationMgr.cpp ConstellationMgr.hpp Meteor.cpp \
	Meteor.hpp MeteorMgr.cpp MeteorMgr.hpp Planet.cpp Planet.hpp orbit.cpp orbit.h \
	SolarSystem.cpp SolarSystem.hpp Telescope.cpp Telescope.hpp TelescopeMgr.cpp \
	TelescopeMgr.hpp Atmosphere.cpp Atmosphere.hpp image.cpp image.h image_mgr.cpp \
	image_mgr.h Landscape.cpp Landscape.hpp LoadingBar.cpp LoadingBar.hpp main.cpp \
	script.cpp script.h script_mgr.cpp script_mgr.h s_gui.cpp s_gui.h Skybright.cpp \
	Skybright.hpp Skylight.cpp Skylight.hpp solve.h StelApp.cpp StelApp.hpp StelAppSdl.cpp \
	audio.cpp audio.h command_interface.cpp command_interface.h \
	stel_command_interface.cpp stel_command_interface.h StelCore.cpp StelCore.hpp stellarium.h \
	stel_ui_conf.cpp stel_ui.cpp stel_ui.h stel_ui_tuiconf.cpp s_tui.cpp s_tui.h \
	ToneReproducer.cpp ToneReproducer.hpp translations.h StelObjectModule.hpp StelObjectModule.cpp \
	StelModule.hpp StelModule.cpp StelModuleMgr.cpp StelModuleMgr.hpp StelFontMgr.hpp StelFontMgr.cpp \
	StelLocaleMgr.hpp StelLocaleMgr.cpp StelSkyCultureMgr.hpp StelSkyCultureMgr.cpp GeodesicGridDrawer.h GeodesicGridDrawer.cpp \
	StelTextureMgr.hpp StelTextureMgr.cpp AstroImage.hpp AstroImage.cpp LandscapeMgr.hpp LandscapeMgr.cpp \
	GridLinesMgr.hpp GridLinesMgr.cpp MilkyWay.hpp MilkyWay.cpp MovementMgr.hpp MovementMgr.cpp \
	StelObjectMgr.hpp StelObjectMgr.cpp

stellarium_LDADD = stelutils/libstelutils.a \
stelutils/iniparser/libiniparser.a \
planetsephems/libplanetsephems.a \
@LIBINTL@

INCLUDES = -I$(top_srcdir)/src/planetsephems -I$(top_srcdir)/src/stelutils -I$(top_srcdir)/intl

SUBDIRS = planetsephems stelutils
noinst_HEADERS = 
