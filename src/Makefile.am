bin_PROGRAMS = stellarium
stellarium_SOURCES = projector.cpp projector.h stereographic_projector.cpp \
	stereographic_projector.h custom_projector.cpp custom_projector.h cylinder_projector.cpp \
	cylinder_projector.h fisheye_projector.cpp fisheye_projector.h spheric_mirror_calculator.cpp \
	spheric_mirror_calculator.h spheric_mirror_projector.cpp spheric_mirror_projector.h \
	viewport_distorter.cpp viewport_distorter.h navigator.cpp navigator.h observator.cpp \
	observator.h grid.cpp grid.h geodesic_grid.cpp geodesic_grid.h \
	stel_object_base.cpp stel_object_base.h stel_object.cpp stel_object.h stel_object_type.h \
	hip_star_mgr.cpp hip_star_mgr.h nebula.cpp nebula.h nebula_mgr.cpp nebula_mgr.h \
	constellation.cpp constellation.h constellation_mgr.cpp constellation_mgr.h meteor.cpp \
	meteor.h meteor_mgr.cpp meteor_mgr.h planet.cpp planet.h orbit.cpp orbit.h \
	solarsystem.cpp solarsystem.h telescope.cpp telescope.h telescope_mgr.cpp \
	telescope_mgr.h stel_atmosphere.cpp stel_atmosphere.h image.cpp image.h image_mgr.cpp \
	image_mgr.h landscape.cpp landscape.h loadingbar.cpp loadingbar.h main.cpp \
	script.cpp script.h script_mgr.cpp script_mgr.h s_gui.cpp s_gui.h skybright.cpp \
	skybright.h skylight.cpp skylight.h solve.h StelApp.cpp StelApp.h stel_sdl.cpp \
	audio.cpp audio.h command_interface.cpp command_interface.h \
	stel_command_interface.cpp stel_command_interface.h stel_core.cpp stel_core.h stellarium.h \
	stel_ui_conf.cpp stel_ui.cpp stel_ui.h stel_ui_tuiconf.cpp s_tui.cpp s_tui.h \
	tone_reproductor.cpp tone_reproductor.h translations.h StelObjectMgr.h StelObjectMgr.cpp \
	StelModule.h StelModule.cpp StelModuleMgr.cpp StelModuleMgr.h StelFontMgr.h StelFontMgr.cpp \
	StelLocaleMgr.h StelLocaleMgr.cpp StelSkyCultureMgr.h StelSkyCultureMgr.cpp GeodesicGridDrawer.h GeodesicGridDrawer.cpp \
	StelTextureMgr.h StelTextureMgr.cpp AstroImage.h AstroImage.cpp LandscapeMgr.h LandscapeMgr.cpp \
	GridLinesMgr.h GridLinesMgr.cpp MilkyWay.h MilkyWay.cpp

stellarium_LDADD = stelutils/iniparser/libiniparser.a \
stelutils/libstelutils.a \
planetsephems/libplanetsephems.a \
@LIBINTL@

INCLUDES = -I$(top_srcdir)/src/planetsephems -I$(top_srcdir)/src/stelutils -I$(top_srcdir)/intl

SUBDIRS = planetsephems stelutils
noinst_HEADERS = 
