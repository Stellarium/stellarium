#INCLUDE_DIRECTORIES(. gui ${CMAKE_BINARY_DIR}/plugins/Planes/src ${CMAKE_BINARY_DIR}/plugins/Planes/src/gui)
INCLUDE_DIRECTORIES(. gui)
LINK_DIRECTORIES(${CMAKE_BINARY_DIR}/src)

SET(Planes_SRCS
  gui/PlanesDialog.hpp
  gui/PlanesDialog.cpp
  FlightDataSource.hpp
  BSDataSource.hpp
  BSDataSource.cpp
  BSRecordingDataSource.hpp
  BSRecordingDataSource.cpp
  DatabaseWorker.hpp
  DatabaseWorker.cpp
  DBCredentials.hpp
  ADS-B.hpp
  ADS-B.cpp
  FlightMgr.hpp
  FlightMgr.cpp
  Flight.hpp
  Flight.cpp
  Planes.hpp
  Planes.cpp)

SET(QT_USE_QTSQL TRUE)

################# compiles resources files ############
SET(Planes_RES ../Planes.qrc)
SET(Planes_UIS gui/PlanesDialog.ui)

IF (${QT_VERSION_MAJOR} EQUAL "5")
     QT5_WRAP_UI(Planes_UIS_H ${Planes_UIS})
     QT5_ADD_RESOURCES(Planes_RES_CXX ${Planes_RES})
ELSE()
     QT_WRAP_UI(Planes_UIS_H ${Planes_UIS})
     QT_ADD_RESOURCES(Planes_RES_CXX ${Planes_RES})
ENDIF()

#SET(extLinkerOption ${OPENGL_LIBRARIES})

ADD_LIBRARY(Planes-static STATIC ${Planes_SRCS} ${Planes_RES_CXX} ${Planes_UIS_H})
#TARGET_LINK_LIBRARIES(Planes-static ${extLinkerOption})
TARGET_LINK_LIBRARIES(Planes-static Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Network Qt${QT_VERSION_MAJOR}::Sql)
#QT5_USE_MODULES(Planes-static Core Declarative Network Widgets OpenGL Sql)
# The library target "Planes-static" has a default OUTPUT_NAME of "Planes-static", so change it.
SET_TARGET_PROPERTIES(Planes-static PROPERTIES OUTPUT_NAME "Planes")
SET_TARGET_PROPERTIES(Planes-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
ADD_DEPENDENCIES(AllStaticPlugins Planes-static)

SET_TARGET_PROPERTIES(Planes-static PROPERTIES FOLDER "plugins/Planes")
