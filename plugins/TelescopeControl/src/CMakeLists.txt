
INCLUDE_DIRECTORIES(
  ${CMAKE_BINARY_DIR}
  ${CMAKE_BINARY_DIR}/plugins/TelescopeControl/src ${CMAKE_BINARY_DIR}/plugins/TelescopeControl/src/gui ${CMAKE_BINARY_DIR}/plugins/TelescopeControl/src/servers
  . gui clients servers)

LINK_DIRECTORIES(/src)

SET(TelescopeControl_SRCS
  TelescopeControlGlobals.hpp
  clients/InterpolatedPosition.hpp
  clients/InterpolatedPosition.cpp
  clients/TelescopeClient.hpp
  clients/TelescopeClient.cpp
  clients/TelescopeClientDirectLx200.hpp
  clients/TelescopeClientDirectLx200.cpp
  clients/TelescopeClientDirectNexStar.hpp
  clients/TelescopeClientDirectNexStar.cpp
  TelescopeControl.hpp
  TelescopeControl.cpp
  gui/SlewDialog.hpp
  gui/SlewDialog.cpp
  gui/TelescopeDialog.hpp
  gui/TelescopeDialog.cpp
  gui/TelescopeConfigurationDialog.hpp
  gui/TelescopeConfigurationDialog.cpp
  gui/StoredPointsDialog.hpp
  gui/StoredPointsDialog.cpp
  servers/LogFile.hpp
  servers/LogFile.cpp
  servers/Socket.hpp
  servers/Socket.cpp
  servers/Server.hpp
  servers/Server.cpp
  servers/Connection.hpp
  servers/Connection.cpp
  servers/SerialPort.hpp
  servers/SerialPort.cpp
  servers/Lx200Connection.hpp
  servers/Lx200Connection.cpp
  servers/Lx200Command.hpp
  servers/Lx200Command.cpp
  servers/NexStarConnection.hpp
  servers/NexStarConnection.cpp
  servers/NexStarCommand.hpp
  servers/NexStarCommand.cpp
  )

# Add here all the .ui files
SET(TelescopeControl_UIS
  gui/slewDialog.ui
  gui/telescopeDialog.ui
  gui/telescopeConfigurationDialog.ui
  gui/storedPointsDialog.ui)

QT5_WRAP_UI(TelescopeControl_UIS_H ${TelescopeControl_UIS})

# Compile the resource files
SET(TelescopeControl_RES ../resources/TelescopeControl.qrc)
QT5_ADD_RESOURCES(TelescopeControl_RES_CXX ${TelescopeControl_RES})

SET(extLinkerOption ${OPENGL_LIBRARIES})

ADD_LIBRARY(TelescopeControl-static STATIC ${TelescopeControl_SRCS} ${TelescopeControl_RES_CXX} ${TelescopeControl_UIS_H})
SET_TARGET_PROPERTIES(TelescopeControl-static PROPERTIES OUTPUT_NAME "TelescopeControl")
TARGET_LINK_LIBRARIES(TelescopeControl-static ${extLinkerOption})
QT5_USE_MODULES(TelescopeControl-static Core Network Widgets Declarative OpenGL)
SET_TARGET_PROPERTIES(TelescopeControl-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
ADD_DEPENDENCIES(AllStaticPlugins TelescopeControl-static)
