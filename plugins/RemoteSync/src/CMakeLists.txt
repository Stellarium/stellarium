INCLUDE_DIRECTORIES(. gui)
LINK_DIRECTORIES(${CMAKE_BINARY_DIR}/src)

SET(RemoteSync_SRCS
  RemoteSync.hpp
  RemoteSync.cpp
  SyncClient.hpp
  SyncClient.cpp
  SyncClientHandlers.hpp
  SyncClientHandlers.cpp
  SyncMessages.hpp
  SyncMessages.cpp
  SyncProtocol.hpp
  SyncProtocol.cpp
  SyncServer.hpp
  SyncServer.cpp
  SyncServerEventSenders.hpp
  SyncServerEventSenders.cpp
  SyncServerHandlers.hpp
  SyncServerHandlers.cpp
  gui/RemoteSyncDialog.hpp
  gui/RemoteSyncDialog.cpp
)

SET(RemoteSync_UIS
  gui/remoteSyncDialog.ui
)

################# compiles resources files ############
SET(RemoteSync_RES ../RemoteSync.qrc)

IF (${QT_VERSION_MAJOR} EQUAL "5")
     QT5_WRAP_UI(RemoteSync_UIS_H ${RemoteSync_UIS})
     QT5_ADD_RESOURCES(RemoteSync_RES_CXX ${RemoteSync_RES})
ELSE()
     QT_WRAP_UI(RemoteSync_UIS_H ${RemoteSync_UIS})
     QT_ADD_RESOURCES(RemoteSync_RES_CXX ${RemoteSync_RES})
ENDIF()

ADD_LIBRARY(RemoteSync-static STATIC ${RemoteSync_SRCS} ${RemoteSync_UIS_H} ${RemoteSync_RES_CXX})
TARGET_LINK_LIBRARIES(RemoteSync-static Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Network Qt${QT_VERSION_MAJOR}::Widgets)
# The library target "RemoteSync-static" has a default OUTPUT_NAME of "RemoteSync-static", so change it.
SET_TARGET_PROPERTIES(RemoteSync-static PROPERTIES OUTPUT_NAME "RemoteSync")
SET_TARGET_PROPERTIES(RemoteSync-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
ADD_DEPENDENCIES(AllStaticPlugins RemoteSync-static)

SET_TARGET_PROPERTIES(RemoteSync-static PROPERTIES FOLDER "plugins/RemoteSync")
