INCLUDE_DIRECTORIES(. gui qtwebapp)
LINK_DIRECTORIES(${BUILD_DIR}/src)

SET(QtWebApp_SRCS
  qtwebapp/httpserver/httpconnectionhandler.cpp
  qtwebapp/httpserver/httpconnectionhandler.h
  qtwebapp/httpserver/httpconnectionhandlerpool.cpp
  qtwebapp/httpserver/httpconnectionhandlerpool.h
  qtwebapp/httpserver/httpcookie.cpp
  qtwebapp/httpserver/httpcookie.h
  qtwebapp/httpserver/httpglobal.cpp
  qtwebapp/httpserver/httpglobal.h
  qtwebapp/httpserver/httplistener.cpp
  qtwebapp/httpserver/httplistener.h
  qtwebapp/httpserver/httprequest.cpp
  qtwebapp/httpserver/httprequest.h
  qtwebapp/httpserver/httprequesthandler.cpp
  qtwebapp/httpserver/httprequesthandler.h
  qtwebapp/httpserver/httpresponse.cpp
  qtwebapp/httpserver/httpresponse.h
  qtwebapp/httpserver/staticfilecontroller.cpp
  qtwebapp/httpserver/staticfilecontroller.h
  
  qtwebapp/templateengine/template.cpp
  qtwebapp/templateengine/template.h
  qtwebapp/templateengine/templateglobal.h
)

SET(RemoteControl_SRCS
  AbstractAPIService.hpp
  AbstractAPIService.cpp
  APIController.hpp
  APIController.cpp
  MainService.hpp
  MainService.cpp
  ObjectService.hpp
  ObjectService.cpp
  LocationService.hpp
  LocationService.cpp
  LocationSearchService.hpp
  LocationSearchService.cpp
  RemoteControl.hpp
  RemoteControl.cpp
  RequestHandler.hpp
  RequestHandler.cpp
  ScriptService.hpp
  ScriptService.cpp
  SimbadService.hpp
  SimbadService.cpp
  StelActionService.hpp
  StelActionService.cpp
  StelPropertyService.hpp
  StelPropertyService.cpp
  ViewService.hpp
  ViewService.cpp
  gui/RemoteControlDialog.hpp
  gui/RemoteControlDialog.cpp
)

SET(RemoteControl_UIS
  gui/remoteControlDialog.ui
)

QT5_WRAP_UI(RemoteControl_UIS_H ${RemoteControl_UIS})

################# compiles resources files ############
SET(RemoteControl_RES ../RemoteControl.qrc)
QT5_ADD_RESOURCES(RemoteControl_RES_CXX ${RemoteControl_RES})

SET(extLinkerOption ${JPEG_LIBRARIES} ${PNG_LIBRARIES} ${OPENGL_LIBRARIES})

ADD_LIBRARY(RemoteControl-static STATIC ${RemoteControl_SRCS} ${RemoteControl_RES_CXX} ${RemoteControl_UIS_H} ${QtWebApp_SRCS})
QT5_USE_MODULES(RemoteControl-static Core Network Widgets)
# The library target "RemoteControl-static" has a default OUTPUT_NAME of "RemoteControl-static", so change it.
SET_TARGET_PROPERTIES(RemoteControl-static PROPERTIES OUTPUT_NAME "RemoteControl")
TARGET_LINK_LIBRARIES(RemoteControl-static ${extLinkerOption})
SET_TARGET_PROPERTIES(RemoteControl-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
ADD_DEPENDENCIES(AllStaticPlugins RemoteControl-static)
