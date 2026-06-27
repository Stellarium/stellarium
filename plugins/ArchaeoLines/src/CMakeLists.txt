INCLUDE_DIRECTORIES(. gui)
LINK_DIRECTORIES(${CMAKE_BINARY_DIR}/src)

SET(ArchaeoLines_SRCS
     ArchaeoLines.hpp
     ArchaeoLines.cpp
)
IF (STELLARIUM_GUI_MODE STREQUAL "Standard")
LIST(APPEND ArchaeoLines_SRCS
     gui/ArchaeoLinesDialog.hpp
     gui/ArchaeoLinesDialog.cpp
     gui/ArchaeoLinesDialogLocations.hpp
     gui/ArchaeoLinesDialogLocations.cpp
)

SET(ArchaeoLines_UIS
     gui/archaeoLinesDialog.ui
     gui/archaeoLinesDialogLocations.ui
)
ENDIF (STELLARIUM_GUI_MODE STREQUAL "Standard")

################# compiles resources files ############
SET(ArchaeoLines_RES ../ArchaeoLines.qrc)
IF (${QT_VERSION_MAJOR} EQUAL "5")
     IF (STELLARIUM_GUI_MODE STREQUAL "Standard")
          QT5_WRAP_UI(ArchaeoLines_UIS_H ${ArchaeoLines_UIS})
     ENDIF()
     QT5_ADD_RESOURCES(ArchaeoLines_RES_CXX ${ArchaeoLines_RES})
ELSE()
     IF (STELLARIUM_GUI_MODE STREQUAL "Standard")
           QT_WRAP_UI(ArchaeoLines_UIS_H ${ArchaeoLines_UIS})
     ENDIF()
     QT_ADD_RESOURCES(ArchaeoLines_RES_CXX ${ArchaeoLines_RES})
ENDIF()

ADD_LIBRARY(ArchaeoLines-static STATIC ${ArchaeoLines_SRCS} ${ArchaeoLines_RES_CXX} ${ArchaeoLines_UIS_H})
TARGET_LINK_LIBRARIES(ArchaeoLines-static Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::OpenGL)
# The library target "ArchaeoLines-static" has a default OUTPUT_NAME of "ArchaeoLines-static", so change it.
SET_TARGET_PROPERTIES(ArchaeoLines-static PROPERTIES OUTPUT_NAME "ArchaeoLines")
SET_TARGET_PROPERTIES(ArchaeoLines-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
ADD_DEPENDENCIES(AllStaticPlugins ArchaeoLines-static)

SET_TARGET_PROPERTIES(ArchaeoLines-static PROPERTIES FOLDER "plugins/ArchaeoLines")
