bin_PROGRAMS = stellarium
stellarium_SOURCES = main.cpp stel_sdl.cpp stel_core.cpp \
  stel_utility.cpp stel_object.cpp \
  stel_atmosphere.cpp grid.cpp constellation.cpp \
  constellation_mgr.cpp nebula.cpp nebula_mgr.cpp planet.cpp \
  solarsystem.cpp hip_star.cpp hip_star_mgr.cpp \
  navigator.cpp observator.cpp draw.cpp s_texture.cpp \
  orbit.cpp skylight.cpp skybright.cpp \
  tone_reproductor.cpp init_parser.cpp s_gui.cpp \
  stel_ui.cpp stel_ui_conf.cpp stel_ui_tuiconf.cpp \
  projector.cpp custom_projector.cpp \
  fisheye_projector.cpp cylinder_projector.cpp \
  stereographic_projector.cpp spheric_mirror_projector.cpp \
  landscape.cpp s_tui.cpp \
  meteor.cpp \
  meteor_mgr.cpp sky_localizer.cpp \
  command_interface.cpp stel_command_interface.cpp \
  script_mgr.cpp script.cpp image_mgr.cpp image.cpp \
  audio.cpp loadingbar.cpp \
  typeface.h typeface.cpp translator.cpp


stellarium_LDADD = $(top_builddir)/src/glpng/libglpng.a \
$(top_builddir)/src/stellplanet/libstellplanet.a \
$(top_builddir)/src/stellastro/libstellastro.a \
$(top_builddir)/src/iniparser/libiniparser.a \
@LIBINTL@

INCLUDES = -I$(top_srcdir)/src/glpng -I$(top_srcdir)/src/stellplanet -I$(top_srcdir)/src/stellastro -I$(top_srcdir)/src/iniparser -I$(top_srcdir)/intl

SUBDIRS = glpng stellplanet stellastro iniparser
noinst_HEADERS = translations.h translator.h
