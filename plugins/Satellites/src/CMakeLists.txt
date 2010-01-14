
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/src 
  ${CMAKE_SOURCE_DIR}/src/sgp4sdp4
  ${STELLARIUM_BUILD_DIR}
  ${STELLARIUM_SOURCE_DIR}
  ${STELLARIUM_SOURCE_DIR}/core
  ${STELLARIUM_SOURCE_DIR}/gui
  ${STELLARIUM_SOURCE_DIR}/modules
  ${STELLARIUM_SOURCE_DIR}/stelutils
  ${STELLARIUM_SOURCE_DIR}/external
  ${STELLARIUM_SOURCE_DIR}/scripting
  ${CMAKE_BINARY_DIR}
  ${CMAKE_BINARY_DIR}/src
  ${CMAKE_SOURCE_DIR}/src 
  ${CMAKE_SOURCE_DIR}/src/gui
  ${OPENGL_INCLUDE_DIR}
  ${Boost_INCLUDE_DIRS})

LINK_DIRECTORIES(${STELLARIUM_BUILD_DIR}/src)

SET(Satellites_SRCS
  sgp4sdp4/sgp4sdp4.c
  sgp4sdp4/sgp4sdp4.h
  sgp4sdp4/sgp_in.c
  sgp4sdp4/sgp_math.c
  sgp4sdp4/sgp_obs.c
  sgp4sdp4/sgp_time.c
  sgp4sdp4/solar.c
  Satellite.hpp
  Satellite.cpp
  Satellites.hpp
  Satellites.cpp
  gui/StelDialogPlugin.hpp
  gui/StelDialogPlugin.cpp
  gui/SatellitesDialog.hpp
  gui/SatellitesDialog.cpp
)

SET(SatellitesDialog_UIS
  gui/satellitesDialog.ui
)
QT4_WRAP_UI(SatellitesDialog_UIS_H ${SatellitesDialog_UIS})

SET(Satellites_RES
  ${CMAKE_SOURCE_DIR}/Satellites.qrc)
QT4_ADD_RESOURCES(Satellites_RES_CXX ${Satellites_RES})

# Add here all the files which require moc compilation
# that is, all the headers with SIGNAL/SLOTS/PLUGIN_INTERFACE etc..
SET(Satellites_MOC_HDRS
  Satellites.hpp
  gui/StelDialogPlugin.hpp
  gui/SatellitesDialog.hpp
)
 
# After this call, Satellites_MOC_SRCS = moc_Satellites.cxx
QT4_WRAP_CPP(Satellites_MOC_SRCS ${Satellites_MOC_HDRS})

# Dummy target to trigger ui compilations
ADD_CUSTOM_TARGET(GenerateUiHeaders ALL DEPENDS ${SatellitesDialog_UIS_H})

ADD_LIBRARY(Satellites MODULE ${Satellites_SRCS} ${Satellites_MOC_SRCS} ${Satellites_RES_CXX})
ADD_LIBRARY(Satellites-static STATIC ${Satellites_SRCS} ${Satellites_MOC_SRCS} ${Satellites_RES_CXX})
# The library target "Satellites-static" has a default OUTPUT_NAME of "Satellites-static", so change it.
SET_TARGET_PROPERTIES(Satellites-static PROPERTIES OUTPUT_NAME "Satellites-static")

IF(APPLE)
  FIND_LIBRARY(OPENGL_LIBRARY OpenGL)
  MARK_AS_ADVANCED(OPENGL_LIBRARY)
  SET_TARGET_PROPERTIES(Satellites PROPERTIES LINK_FLAGS "-undefined dynamic_lookup" SUFFIX ".dylib")
ENDIF(APPLE)

IF(WIN32)
  SET_TARGET_PROPERTIES(Satellites PROPERTIES LINK_FLAGS "-enable-runtime-pseudo-reloc -Wl,--allow-multiple-definition" )
  SET(StelMain stelMain)
ELSE(WIN32)
  SET(StelMain )
ENDIF(WIN32)

SET(extLinkerOption ${QT_LIBRARIES} ${FreeType2_LIBRARIES} ${JPEG_LIBRARIES} ${PNG_LIBRARIES} ${OPENGL_LIBRARIES} ${ICONV_LIBRARIES} ${INTL_LIBRARIES})

TARGET_LINK_LIBRARIES(Satellites ${StelMain} ${extLinkerOption})
TARGET_LINK_LIBRARIES(Satellites-static ${StelMain} ${extLinkerOption})
SET_TARGET_PROPERTIES(Satellites-static PROPERTIES COMPILE_FLAGS "-fPIC -DQT_STATICPLUGIN")

INSTALL(TARGETS Satellites DESTINATION "modules/${PACKAGE}")

