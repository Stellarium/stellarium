INCLUDE_DIRECTORIES(. gui)
LINK_DIRECTORIES(${BUILD_DIR}/src)

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

QT5_WRAP_UI(RemoteSync_UIS_H ${RemoteSync_UIS})

################# compiles resources files ############
SET(RemoteSync_RES ../RemoteSync.qrc)
QT5_ADD_RESOURCES(RemoteSync_RES_CXX ${RemoteSync_RES})

SET(extLinkerOption ${JPEG_LIBRARIES} ${PNG_LIBRARIES} ${OPENGL_LIBRARIES})

ADD_LIBRARY(RemoteSync-static STATIC ${RemoteSync_SRCS} ${RemoteSync_UIS_H} ${RemoteSync_RES_CXX})
QT5_USE_MODULES(RemoteSync-static Core Network Widgets)
# The library target "RemoteSync-static" has a default OUTPUT_NAME of "RemoteSync-static", so change it.
SET_TARGET_PROPERTIES(RemoteSync-static PROPERTIES OUTPUT_NAME "RemoteSync")
TARGET_LINK_LIBRARIES(RemoteSync-static ${extLinkerOption})
SET_TARGET_PROPERTIES(RemoteSync-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
ADD_DEPENDENCIES(AllStaticPlugins RemoteSync-static)
