EXTRA_DIST = stellarium.rc

bin_PROGRAMS = stellarium
stellarium_SOURCES = \
main.cpp \
stel_sdl.h          stel_sdl.cpp \
stel_core.h         stel_core.cpp \
stel_utility.h      stel_utility.cpp \
stel_object.h       stel_object.cpp \
stel_atmosphere.h   stel_atmosphere.cpp \
grid.h              grid.cpp \
constellation.h     constellation.cpp \
constellation_mgr.h constellation_mgr.cpp \
nebula.h            nebula.cpp \
nebula_mgr.h        nebula_mgr.cpp \
planet.h            planet.cpp \
solarsystem.h       solarsystem.cpp \
hip_star.h          hip_star.cpp \
hip_star_mgr.h      hip_star_mgr.cpp \
navigator.h         navigator.cpp \
draw.h              draw.cpp \
s_texture.h         s_texture.cpp \
s_font.h            s_font.cpp \
orbit.h	solve.h		orbit.cpp \
skylight.h          skylight.cpp \
tone_reproductor.h  tone_reproductor.cpp \
init_parser.h       init_parser.cpp \
stellarium.h \
vecmath.h bytes.h

stellarium_LDADD = $(top_builddir)/src/glpng/libglpng.a \
$(top_builddir)/src/stellplanet/libstellplanet.a \
$(top_builddir)/src/stellastro/libstellastro.a \
$(top_builddir)/src/iniparser/libiniparser.a


INCLUDES = -I$(top_srcdir)/src/glpng -I$(top_srcdir)/src/stellplanet -I$(top_srcdir)/src/stellastro -I$(top_srcdir)/src/iniparser

SUBDIRS = glpng stellplanet stellastro iniparser
