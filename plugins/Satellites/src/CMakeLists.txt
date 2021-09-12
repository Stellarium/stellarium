INCLUDE_DIRECTORIES(
     .
     gsatellite
     gui
     ${CMAKE_BINARY_DIR}/plugins/Satellites/src
     ${CMAKE_BINARY_DIR}/plugins/Satellites/src/gui
)

LINK_DIRECTORIES(${BUILD_DIR}/src)

SET(Satellites_SRCS
     gsatellite/gException.hpp
     gsatellite/gSatTEME.cpp
     gsatellite/gSatTEME.hpp
     gsatellite/mathUtils.cpp
     gsatellite/mathUtils.hpp
     gsatellite/gTime.cpp
     gsatellite/gTime.hpp
     gsatellite/gTimeSpan.cpp
     gsatellite/sgp4ext.cpp
     gsatellite/sgp4ext.h
     gsatellite/sgp4io.cpp
     gsatellite/sgp4io.h
     gsatellite/sgp4unit.cpp
     gsatellite/sgp4unit.h
     gsatellite/stdsat.h

     gSatWrapper.hpp
     gSatWrapper.cpp
     Satellite.hpp
     Satellite.cpp
     Satellites.hpp
     Satellites.cpp
     SatellitesListModel.hpp
     SatellitesListModel.cpp
     SatellitesListFilterModel.hpp
     SatellitesListFilterModel.cpp
     gui/SatellitesDialog.hpp
     gui/SatellitesDialog.cpp
     gui/SatellitesImportDialog.hpp
     gui/SatellitesImportDialog.cpp
)

SET(SatellitesDialog_UIS
     gui/satellitesDialog.ui
     gui/satellitesImportDialog.ui
)
QT5_WRAP_UI(SatellitesDialog_UIS_H ${SatellitesDialog_UIS})

SET(Satellites_RES ../resources/Satellites.qrc)
QT5_ADD_RESOURCES(Satellites_RES_CXX ${Satellites_RES})

IF(ENABLE_TESTING)
    ADD_SUBDIRECTORY(test)
ENDIF(ENABLE_TESTING)

ADD_LIBRARY(Satellites-static STATIC ${Satellites_SRCS} ${Satellites_RES_CXX} ${SatellitesDialog_UIS_H})
TARGET_LINK_LIBRARIES(Satellites-static Qt5::Core Qt5::Network Qt5::Widgets)
# The library target "Satellites-static" has a default OUTPUT_NAME of "Satellites-static", so change it.
SET_TARGET_PROPERTIES(Satellites-static PROPERTIES OUTPUT_NAME "Satellites")
IF(MSVC)
     SET_TARGET_PROPERTIES(Satellites-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
ELSE()
     SET_TARGET_PROPERTIES(Satellites-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN -Wno-unused-parameter")
ENDIF()
ADD_DEPENDENCIES(AllStaticPlugins Satellites-static)

SET_TARGET_PROPERTIES(Satellites-static PROPERTIES FOLDER "plugins/Satellites")
