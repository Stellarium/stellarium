noinst_LIBRARIES = libstelutils.a

libstelutils_a_SOURCES = bytes.h gettext.h init_parser.cpp init_parser.h s_font.h s_texture.cpp \
	s_texture.h typeface.cpp typeface.h stel_utility.cpp stel_utility.h translator.cpp \
	translator.h vecmath.h callbacks.hpp callback_helpers.hpp glpng/glpng.c \
	iniparser/dictionary.c iniparser/iniparser.c iniparser/strlib.c 

INCLUDES = -I$(top_srcdir)/src/stelutils/iniparser -I$(top_srcdir)/src/stelutils/glpng

SUBDIRS = glpng iniparser
