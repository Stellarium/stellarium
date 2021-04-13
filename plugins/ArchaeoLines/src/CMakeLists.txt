INCLUDE_DIRECTORIES(. gui)
LINK_DIRECTORIES(${BUILD_DIR}/src)

SET(ArchaeoLines_SRCS
     ArchaeoLines.hpp
     ArchaeoLines.cpp
     gui/ArchaeoLinesDialog.hpp
     gui/ArchaeoLinesDialog.cpp
)

SET(ArchaeoLines_UIS
     gui/archaeoLinesDialog.ui
)

QT5_WRAP_UI(ArchaeoLines_UIS_H ${ArchaeoLines_UIS})

################# compiles resources files ############
SET(ArchaeoLines_RES ../ArchaeoLines.qrc)
QT5_ADD_RESOURCES(ArchaeoLines_RES_CXX ${ArchaeoLines_RES})

ADD_LIBRARY(ArchaeoLines-static STATIC ${ArchaeoLines_SRCS} ${ArchaeoLines_RES_CXX} ${ArchaeoLines_UIS_H})
TARGET_LINK_LIBRARIES(ArchaeoLines-static Qt5::Core Qt5::Widgets)
# The library target "ArchaeoLines-static" has a default OUTPUT_NAME of "ArchaeoLines-static", so change it.
SET_TARGET_PROPERTIES(ArchaeoLines-static PROPERTIES OUTPUT_NAME "ArchaeoLines")
SET_TARGET_PROPERTIES(ArchaeoLines-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
ADD_DEPENDENCIES(AllStaticPlugins ArchaeoLines-static)

SET_TARGET_PROPERTIES(ArchaeoLines-static PROPERTIES FOLDER "plugins/ArchaeoLines")
