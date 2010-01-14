
INCLUDE_DIRECTORIES(
  ${TELESCOPE_SERVERS_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}
  ${CMAKE_BINARY_DIR}/plugins/TelescopeControl/src ${CMAKE_BINARY_DIR}/plugins/TelescopeControl/src/gui
  . gui clients)

LINK_DIRECTORIES(/src)

SET(TelescopeControl_SRCS
  TelescopeControlGlobals.hpp
  clients/TelescopeClient.hpp
  clients/TelescopeClient.cpp
  clients/TelescopeClientDirectLx200.hpp
  clients/TelescopeClientDirectLx200.cpp
  clients/TelescopeClientDirectNexStar.hpp
  clients/TelescopeClientDirectNexStar.cpp
  TelescopeControl.hpp
  TelescopeControl.cpp
  gui/TelescopeDialog.hpp
  gui/TelescopeDialog.cpp
  gui/TelescopeConfigurationDialog.hpp
  gui/TelescopeConfigurationDialog.cpp
  gui/StelDialogTelescopeControl.hpp
  gui/StelDialogTelescopeControl.cpp
  ${TELESCOPE_SERVERS_SOURCE_DIR}/LogFile.hpp
  ${TELESCOPE_SERVERS_SOURCE_DIR}/LogFile.cpp
  ${TELESCOPE_SERVERS_SOURCE_DIR}/Socket.hpp
  ${TELESCOPE_SERVERS_SOURCE_DIR}/Socket.cpp
  ${TELESCOPE_SERVERS_SOURCE_DIR}/Server.hpp
  ${TELESCOPE_SERVERS_SOURCE_DIR}/Server.cpp
  ${TELESCOPE_SERVERS_SOURCE_DIR}/Connection.hpp
  ${TELESCOPE_SERVERS_SOURCE_DIR}/Connection.cpp
  ${TELESCOPE_SERVERS_SOURCE_DIR}/Listener.hpp
  ${TELESCOPE_SERVERS_SOURCE_DIR}/Listener.cpp
  ${TELESCOPE_SERVERS_SOURCE_DIR}/SerialPort.hpp
  ${TELESCOPE_SERVERS_SOURCE_DIR}/SerialPort.cpp
  ${TELESCOPE_SERVERS_SOURCE_DIR}/ServerLx200.hpp
  ${TELESCOPE_SERVERS_SOURCE_DIR}/ServerLx200.cpp
  ${TELESCOPE_SERVERS_SOURCE_DIR}/Lx200Connection.hpp
  ${TELESCOPE_SERVERS_SOURCE_DIR}/Lx200Connection.cpp
  ${TELESCOPE_SERVERS_SOURCE_DIR}/Lx200Command.hpp
  ${TELESCOPE_SERVERS_SOURCE_DIR}/Lx200Command.cpp
  ${TELESCOPE_SERVERS_SOURCE_DIR}/ServerNexStar.hpp
  ${TELESCOPE_SERVERS_SOURCE_DIR}/ServerNexStar.cpp
  ${TELESCOPE_SERVERS_SOURCE_DIR}/NexStarConnection.hpp
  ${TELESCOPE_SERVERS_SOURCE_DIR}/NexStarConnection.cpp
  ${TELESCOPE_SERVERS_SOURCE_DIR}/NexStarCommand.hpp
  ${TELESCOPE_SERVERS_SOURCE_DIR}/NexStarCommand.cpp
  )

# Add here all the .ui files
SET(TelescopeControl_UIS
  gui/telescopeDialog.ui
  gui/telescopeConfigurationDialog.ui)

QT4_WRAP_UI(TelescopeControl_UIS_H ${TelescopeControl_UIS})

# Compile the resource files
SET(TelescopeControl_RES ../TelescopeControl.qrc)
QT4_ADD_RESOURCES(TelescopeControl_RES_CXX ${TelescopeControl_RES})

# Add here all the files which require moc compilation
# that is, all the headers with SIGNAL/SLOTS/PLUGIN_INTERFACE etc..
SET(TelescopeControl_MOC_HDRS
  gui/StelDialogTelescopeControl.hpp
  gui/TelescopeConfigurationDialog.hpp
  gui/TelescopeDialog.hpp
  TelescopeControl.hpp
  clients/TelescopeClient.hpp
  clients/TelescopeClientDirectLx200.hpp
  clients/TelescopeClientDirectNexStar.hpp
  )

# After this call, TelescopeControl_MOC_SRCS = moc_TelescopeControl.cxx
QT4_WRAP_CPP(TelescopeControl_MOC_SRCS ${TelescopeControl_MOC_HDRS})

SET(extLinkerOption ${QT_LIBRARIES} ${OPENGL_LIBRARIES} ${ICONV_LIBRARIES} ${INTL_LIBRARIES})

############### For building the dynamic library ######################
IF(BUILD_DYNAMIC_PLUGINS)
 ADD_LIBRARY(TelescopeControl MODULE ${TelescopeControl_SRCS} ${TelescopeControl_MOC_SRCS} ${TelescopeControl_RES_CXX} ${TelescopeControl_UIS_H})

 IF(APPLE)
  FIND_LIBRARY(OPENGL_LIBRARY OpenGL)
  MARK_AS_ADVANCED(OPENGL_LIBRARY)
  SET_TARGET_PROPERTIES(TelescopeControl PROPERTIES LINK_FLAGS "-undefined dynamic_lookup" SUFFIX ".dylib")
 ENDIF(APPLE)

 IF(WIN32)
  SET_TARGET_PROPERTIES(TelescopeControl PROPERTIES LINK_FLAGS "-enable-runtime-pseudo-reloc -Wl,--allow-multiple-definition" )
  SET(StelMain stelMain)
 ELSE(WIN32)
  SET(StelMain )
 ENDIF(WIN32)

 TARGET_LINK_LIBRARIES(TelescopeControl ${StelMain} ${extLinkerOption})

 INSTALL(TARGETS TelescopeControl DESTINATION "modules/TelescopeControl")
ENDIF()

############### For building the static library ######################
IF(BUILD_STATIC_PLUGINS)
 ADD_LIBRARY(TelescopeControl-static STATIC ${TelescopeControl_SRCS} ${TelescopeControl_MOC_SRCS} ${TelescopeControl_RES_CXX} ${TelescopeControl_UIS_H})
 SET_TARGET_PROPERTIES(TelescopeControl-static PROPERTIES OUTPUT_NAME "TelescopeControl")
 TARGET_LINK_LIBRARIES(TelescopeControl-static ${extLinkerOption})
 SET_TARGET_PROPERTIES(TelescopeControl-static PROPERTIES COMPILE_FLAGS "-fPIC -DQT_STATICPLUGIN")
 ADD_DEPENDENCIES(AllStaticPlugins TelescopeControl-static)
ENDIF()
