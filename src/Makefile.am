EXTRA_DIST = stellarium.PRJ stellarium.rc

bin_PROGRAMS = stellarium
stellarium_SOURCES = constellation.h  nebula.h  planet.h s_gui.h  s_utility.h   vector.h constellation_mgr.cpp intl.h  nebula_mgr.cpp  planet_mgr.cpp  s_gui_window.cpp  star.cpp  stellarium.h VISLIMIT.CPP  constellation_mgr.h main.cpp  nebula_mgr.h planet_mgr.h  s_gui_window.h tar.h  VISLIMIT.H  navigation.cpp  parsecfg.c  s_font.cpp    s_texture.cpp star_mgr.cpp   stellarium_ui.cpp draw.cpp navigation.h parsecfg.h s_font.h  s_texture.h   star_mgr.h stellarium_ui.h constellation.cpp  draw.h nebula.cpp  planet.cpp s_gui.cpp  s_utility.cpp stella_types.h  utility.h

stellarium_LDADD = $(top_builddir)/src/glpng/libglpng.a $(top_builddir)/src/astrolib/libastrolib.a

INCLUDES = -I$(top_srcdir)/src/glpng -I$(top_srcdir)/src/astrolib

SUBDIRS = glpng astrolib

