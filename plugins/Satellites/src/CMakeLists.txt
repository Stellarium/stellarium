INCLUDE_DIRECTORIES(
     .
     gsatellite
     gui
     ${CMAKE_BINARY_DIR}/plugins/Satellites/src
     ${CMAKE_BINARY_DIR}/plugins/Satellites/src/gui
)

LINK_DIRECTORIES(${CMAKE_BINARY_DIR}/src)

SET(Satellites_SRCS
     gsatellite/gException.hpp
     gsatellite/gSatTEME.cpp
     gsatellite/gSatTEME.hpp
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
     gui/SatellitesFilterDialog.hpp
     gui/SatellitesFilterDialog.cpp
     gui/SatellitesCommDialog.hpp
     gui/SatellitesCommDialog.cpp
)

SET(SatellitesDialog_UIS
     gui/satellitesDialog.ui
     gui/satellitesImportDialog.ui
     gui/satellitesFilterDialog.ui
     gui/satellitesCommDialog.ui
)

SET(Satellites_RES ../resources/Satellites.qrc)
IF (${QT_VERSION_MAJOR} EQUAL "5")
     QT5_WRAP_UI(SatellitesDialog_UIS_H ${SatellitesDialog_UIS})
     QT5_ADD_RESOURCES(Satellites_RES_CXX ${Satellites_RES})
ELSE()
     QT_WRAP_UI(SatellitesDialog_UIS_H ${SatellitesDialog_UIS})
     QT_ADD_RESOURCES(Satellites_RES_CXX ${Satellites_RES})
ENDIF()

IF(ENABLE_TESTING)
     ADD_SUBDIRECTORY(test)
ENDIF(ENABLE_TESTING)

ADD_LIBRARY(Satellites-static STATIC ${Satellites_SRCS} ${Satellites_RES_CXX} ${SatellitesDialog_UIS_H})
IF(ENABLE_XLSX AND (${SATELLITES_PLUGIN_IRIDIUM} EQUAL "1"))
    TARGET_LINK_LIBRARIES(Satellites-static Qt${QT_VERSION_MAJOR}::Core
	    Qt${QT_VERSION_MAJOR}::Network Qt${QT_VERSION_MAJOR}::Widgets
	    Qt${QT_VERSION_MAJOR}::GuiPrivate QXlsx::QXlsx)
ELSE()
    TARGET_LINK_LIBRARIES(Satellites-static Qt${QT_VERSION_MAJOR}::Core
	    Qt${QT_VERSION_MAJOR}::Network Qt${QT_VERSION_MAJOR}::Widgets
	    Qt${QT_VERSION_MAJOR}::GuiPrivate)
ENDIF()
# The library target "Satellites-static" has a default OUTPUT_NAME of "Satellites-static", so change it.
SET_TARGET_PROPERTIES(Satellites-static PROPERTIES OUTPUT_NAME "Satellites")
IF(MSVC)
     SET_TARGET_PROPERTIES(Satellites-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
ELSE()
     SET_TARGET_PROPERTIES(Satellites-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN -Wno-unused-parameter")
ENDIF()
ADD_DEPENDENCIES(AllStaticPlugins Satellites-static)

SET_TARGET_PROPERTIES(Satellites-static PROPERTIES FOLDER "plugins/Satellites")
