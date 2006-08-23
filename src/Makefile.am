bin_PROGRAMS = stellarium
stellarium_SOURCES = audio.cpp audio.h command_interface.cpp \
	command_interface.h constellation.cpp constellation.h constellation_mgr.cpp \
	constellation_mgr.h custom_projector.cpp custom_projector.h cylinder_projector.cpp \
	cylinder_projector.h draw.cpp draw.h fisheye_projector.cpp fisheye_projector.h grid.cpp \
	grid.h hip_star.cpp hip_star.h hip_star_mgr.cpp hip_star_mgr.h image.cpp \
	image.h image_mgr.cpp image_mgr.h landscape.cpp landscape.h loadingbar.cpp \
	loadingbar.h main.cpp meteor.cpp meteor.h meteor_mgr.cpp meteor_mgr.h navigator.cpp \
	navigator.h nebula.cpp nebula.h nebula_mgr.cpp nebula_mgr.h observator.cpp \
	observator.h orbit.cpp orbit.h planet.cpp planet.h projector.cpp projector.h \
	script.cpp script.h script_mgr.cpp script_mgr.h s_gui.cpp s_gui.h skybright.cpp \
	skybright.h skylight.cpp skylight.h sky_localizer.cpp sky_localizer.h \
	solarsystem.cpp solarsystem.h solve.h spheric_mirror_calculator.cpp \
	spheric_mirror_calculator.h spheric_mirror_projector.cpp spheric_mirror_projector.h stelapp.cpp \
	stelapp.h stel_atmosphere.cpp stel_atmosphere.h stel_command_interface.cpp \
	stel_command_interface.h stel_core.cpp stel_core.h stellarium.h stel_object_base.cpp \
	stel_object_base.h stel_object.cpp stel_object.h stel_object_type.h stel_sdl.cpp \
	stel_ui_conf.cpp stel_ui.cpp stel_ui.h stel_ui_tuiconf.cpp stereographic_projector.cpp \
	stereographic_projector.h s_tui.cpp s_tui.h telescope.cpp telescope.h telescope_mgr.cpp \
	telescope_mgr.h tone_reproductor.cpp tone_reproductor.h translations.h \
	viewport_distorter.cpp viewport_distorter.h

stellarium_LDADD = stelutils/glpng/libglpng.a \
stelutils/iniparser/libiniparser.a \
stelutils/libstelutils.a \
planetsephems/libplanetsephems.a \
@LIBINTL@

INCLUDES = -I$(top_srcdir)/src/planetsephems -I$(top_srcdir)/src/stelutils -I$(top_srcdir)/intl

SUBDIRS = planetsephems stelutils
