INCLUDE_DIRECTORIES(
     .
     gui
     ${CMAKE_BINARY_DIR}
     ${CMAKE_BINARY_DIR}/plugins/VisibilityMap/src
     ${CMAKE_BINARY_DIR}/plugins/VisibilityMap/src/gui
)

LINK_DIRECTORIES(${CMAKE_BINARY_DIR}/src)

SET(DAYLIGHTMAP_SRCS
     VisibilityMap.hpp
     VisibilityMap.cpp
     gui/EarthShadowMapWidget.hpp
     gui/EarthShadowMapWidget.cpp
     gui/SunriseSunsetMapWidget.hpp
     gui/SunriseSunsetMapWidget.cpp
     gui/VisibilityMapDialog.hpp
     gui/VisibilityMapDialog.cpp
     gui/StarVisibilityMapWidget.hpp
     gui/StarVisibilityMapWidget.cpp
     gui/StarCalendarWidget.hpp
     gui/StarCalendarWidget.cpp
     gui/DaylightLengthMapWidget.hpp
     gui/DaylightLengthMapWidget.cpp
)

################# compiles resources files ############
SET(DAYLIGHTMAP_RES ../resources/DaylightMap.qrc)
IF (${QT_VERSION_MAJOR} EQUAL "5")
     QT5_ADD_RESOURCES(DAYLIGHTMAP_RES_CXX ${DAYLIGHTMAP_RES})
ELSE()
     QT_ADD_RESOURCES(DAYLIGHTMAP_RES_CXX ${DAYLIGHTMAP_RES})
ENDIF()

ADD_LIBRARY(VisibilityMap-static STATIC ${DAYLIGHTMAP_SRCS} ${DAYLIGHTMAP_RES_CXX})
TARGET_LINK_LIBRARIES(VisibilityMap-static Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Svg Qt${QT_VERSION_MAJOR}::OpenGL)
SET_TARGET_PROPERTIES(VisibilityMap-static PROPERTIES OUTPUT_NAME "VisibilityMap-static")
SET_TARGET_PROPERTIES(VisibilityMap-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
ADD_DEPENDENCIES(AllStaticPlugins VisibilityMap-static)

SET_TARGET_PROPERTIES(VisibilityMap-static PROPERTIES FOLDER "plugins/VisibilityMap")
