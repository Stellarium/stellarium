bin_PROGRAMS = stellarium
stellarium_SOURCES = main.cpp stel_sdl.cpp stel_core.h stel_core.cpp \
				stel_utility.h stel_utility.cpp stel_object.h stel_object.cpp stel_atmosphere.h \
				stel_atmosphere.cpp grid.h grid.cpp constellation.h constellation.cpp constellation_mgr.h \
				constellation_mgr.cpp nebula.h nebula.cpp nebula_mgr.h nebula_mgr.cpp planet.h planet.cpp \
				solarsystem.h solarsystem.cpp hip_star.h hip_star.cpp hip_star_mgr.h hip_star_mgr.cpp \
				navigator.h navigator.cpp draw.h draw.cpp s_texture.h s_texture.cpp s_font.h \
				orbit.h solve.h orbit.cpp skylight.h skylight.cpp skybright.h skybright.cpp \
				tone_reproductor.h tone_reproductor.cpp init_parser.h init_parser.cpp s_gui.h s_gui.cpp \
				stel_ui.h stel_ui.cpp stel_ui_conf.cpp stel_ui_tuiconf.cpp projector.h \
				projector.cpp custom_projector.cpp custom_projector.h \
  stereographic_projector.cpp spheric_mirror_projector.cpp stereographic_projector.h spheric_mirror_projector.h \
   observator.h observator.cpp fisheye_projector.h fisheye_projector.cpp \
				landscape.h landscape.cpp callbacks.hpp callback_helpers.hpp s_tui.h s_tui.cpp \
				stellarium.h fmath.h vecmath.h bytes.h meteor.h meteor.cpp meteor_mgr.h \
				meteor_mgr.cpp sky_localizer.h sky_localizer.cpp command_interface.h \
				command_interface.cpp stel_command_interface.h stel_command_interface.cpp script_mgr.h \
				script_mgr.cpp script.h script.cpp image_mgr.h image_mgr.cpp image.h image.cpp audio.h \
				audio.cpp loadingbar.h loadingbar.cpp fader.h gettext.h cylinder_projector.cpp \
				cylinder_projector.h typeface.h typeface.cpp translator.cpp stelapp.cpp


stellarium_LDADD = $(top_builddir)/src/glpng/libglpng.a \
$(top_builddir)/src/stellplanet/libstellplanet.a \
$(top_builddir)/src/stellastro/libstellastro.a \
$(top_builddir)/src/iniparser/libiniparser.a \
@LIBINTL@

INCLUDES = -I$(top_srcdir)/src/glpng -I$(top_srcdir)/src/stellplanet -I$(top_srcdir)/src/stellastro -I$(top_srcdir)/src/iniparser -I$(top_srcdir)/intl

SUBDIRS = glpng stellplanet stellastro iniparser
noinst_HEADERS = translations.h translator.h stelapp.h
