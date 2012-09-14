#-------------------------------------------------
#
# Project created by QtCreator 2012-09-10T19:57:27
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = locations-editor
TEMPLATE = app

win32:{
	INCLUDEPATH += ../../src/core/external/kfilter/ \
	               ../../src/core/external/
	LIBS += libiconv libintl libz libwsock32
}

SOURCES += main.cpp\
    LocationListEditor.cpp \
    Location.cpp \
    LocationListModel.cpp \
    ../../src/core/external/kfilter/kzip.cpp \
    ../../src/core/external/kfilter/klimitediodevice.cpp \
    ../../src/core/external/kfilter/kgzipfilter.cpp \
    ../../src/core/external/kfilter/kfilterdev.cpp \
    ../../src/core/external/kfilter/kfilterbase.cpp \
    ../../src/core/external/kfilter/karchive.cpp

win32:{
SOURCES += ../../src/core/external/kdewin32/bind/inet_ntop.c \
 ../../src/core/external/kdewin32/bind/inet_pton.c \
 ../../src/core/external/kdewin32/dirent.c \
 ../../src/core/external/kdewin32/errno.c \
 ../../src/core/external/kdewin32/fsync.c \
 ../../src/core/external/kdewin32/getenv.c \
 ../../src/core/external/kdewin32/grp.c \
 ../../src/core/external/kdewin32/inet.c \
 ../../src/core/external/kdewin32/mmap.c \
 ../../src/core/external/kdewin32/nl_langinfo.c \
 ../../src/core/external/kdewin32/net.c \
 ../../src/core/external/kdewin32/pwd.c \
 ../../src/core/external/kdewin32/realpath.c \
 ../../src/core/external/kdewin32/resource.c \
 ../../src/core/external/kdewin32/signal.c \
 ../../src/core/external/kdewin32/stdlib.c \
 ../../src/core/external/kdewin32/string.c \
 ../../src/core/external/kdewin32/strptime.c \
 ../../src/core/external/kdewin32/syslog.c \
 ../../src/core/external/kdewin32/time.c \
 ../../src/core/external/kdewin32/uname.c \
 ../../src/core/external/kdewin32/unistd.c
}
    
HEADERS  += LocationListEditor.hpp \
    Location.hpp \
    LocationListModel.hpp \
    ../../src/core/external/kfilter/kzip.h \
    ../../src/core/external/kfilter/klimitediodevice.h \
    ../../src/core/external/kfilter/kgzipfilter.h \
    ../../src/core/external/kfilter/kfilterdev.h \
    ../../src/core/external/kfilter/kfilterbase.h \
    ../../src/core/external/kfilter/karchive.h

win32:{
HEADERS += ../../src/core/external/kdewin32/basetyps.h \
		../../src/core/external/kdewin32/byteswap.h \
		../../src/core/external/kdewin32/comcat.h \
		../../src/core/external/kdewin32/dirent.h \
		../../src/core/external/kdewin32/docobj.h \
		../../src/core/external/kdewin32/errno.h \
		../../src/core/external/kdewin32/fcntl.h \
		../../src/core/external/kdewin32/grp.h \
		../../src/core/external/kdewin32/ifaddrs.h \
		../../src/core/external/kdewin32/langinfo.h \
		../../src/core/external/kdewin32/mshtml.h \
		../../src/core/external/kdewin32/netdb.h \
		../../src/core/external/kdewin32/nl_types.h \
		../../src/core/external/kdewin32/oaidl.h \
		../../src/core/external/kdewin32/objfwd.h \
		../../src/core/external/kdewin32/objidl.h \
		../../src/core/external/kdewin32/ocidl.h \
		../../src/core/external/kdewin32/olectl.h \
		../../src/core/external/kdewin32/oleidl.h \
		../../src/core/external/kdewin32/pwd.h \
		../../src/core/external/kdewin32/signal.h \
		../../src/core/external/kdewin32/stdio.h \
		../../src/core/external/kdewin32/stdlib.h \
		../../src/core/external/kdewin32/string.h \
		../../src/core/external/kdewin32/strings.h \
		../../src/core/external/kdewin32/syslog.h \
		../../src/core/external/kdewin32/time.h \
		../../src/core/external/kdewin32/unistd.h \
		../../src/core/external/kdewin32/unknwn.h \
		../../src/core/external/kdewin32/wchar.h \
		../../src/core/external/kdewin32/arpa/inet.h \
		../../src/core/external/kdewin32/asm/byteorder.h \
		../../src/core/external/kdewin32/net/if.h \
		../../src/core/external/kdewin32/netinet/in.h \
		../../src/core/external/kdewin32/netinet/tcp.h \
		../../src/core/external/kdewin32/sys/ioctl.h \
		../../src/core/external/kdewin32/sys/mman.h \
		../../src/core/external/kdewin32/sys/resource.h \
		../../src/core/external/kdewin32/sys/select.h \
		../../src/core/external/kdewin32/sys/signal.h \
		../../src/core/external/kdewin32/sys/socket.h \
		../../src/core/external/kdewin32/sys/stat.h \
		../../src/core/external/kdewin32/sys/time.h \
		../../src/core/external/kdewin32/sys/times.h \
		../../src/core/external/kdewin32/sys/types.h \
		../../src/core/external/kdewin32/sys/uio.h \
		../../src/core/external/kdewin32/sys/un.h \
		../../src/core/external/kdewin32/sys/unistd.h \
		../../src/core/external/kdewin32/sys/utsname.h \
		../../src/core/external/kdewin32/sys/wait.h \
}

FORMS    += LocationListEditor.ui

RESOURCES += \
    ../../data/locationsEditor.qrc
