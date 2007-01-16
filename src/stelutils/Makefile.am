noinst_LIBRARIES = libstelutils.a

libstelutils_a_SOURCES = bytes.h gettext.h InitParser.cpp InitParser.hpp SFont.hpp STexture.cpp \
	STexture.hpp typeface.cpp typeface.h StelUtils.cpp StelUtils.hpp Translator.cpp \
	Translator.hpp vecmath.h callbacks.hpp callback_helpers.hpp \
	SphereGeometry.h SphereGeometry.cpp Fader.hpp

INCLUDES = -I$(top_srcdir)/src/stelutils/iniparser

libstelutils_a_LIBADD = iniparser/libiniparser.a

SUBDIRS = iniparser
