INCLUDE_DIRECTORIES(
     .
     gui
     ${CMAKE_BINARY_DIR}
     ${CMAKE_BINARY_DIR}/plugins/OnlineQueries/src
     ${CMAKE_BINARY_DIR}/plugins/OnlineQueries/src/gui
)

LINK_DIRECTORIES(${BUILD_DIR}/src)

SET(ONLINEQUERIES_SRCS
     OnlineQueries.hpp
     OnlineQueries.cpp
     HipOnlineQuery.hpp
     HipOnlineQuery.cpp
     gui/OnlineQueriesDialog.hpp
     gui/OnlineQueriesDialog.cpp
     gui/StelWebEngineView.hpp
     gui/StelWebEngineView.cpp
)

SET(ONLINEQUERIES_UIS
     gui/onlineQueriesDialog.ui
)

QT5_WRAP_UI(ONLINEQUERIES_UIS_H ${ONLINEQUERIES_UIS})

FIND_PACKAGE(Qt5WebEngine QUIET)
FIND_PACKAGE(Qt5WebEngineWidgets QUIET)
IF(Qt5WebEngine_FOUND AND Qt5WebEngineWidgets_FOUND)
    ADD_DEFINITIONS(-DWITH_QTWEBENGINE)
    SET(QTWEBENGINE_LIBS Qt5::WebEngine Qt5::WebEngineWidgets)
ELSE(Qt5WebEngine_FOUND AND Qt5WebEngineWidgets_FOUND)
    MESSAGE(WARNING "Qt5WebEngine and/or Qt5WebEngineWidgets not found. \
                     If these packages exist for your platform, install and rerun. \
                     Else the program will call the system web browser where required.")
ENDIF(Qt5WebEngine_FOUND AND Qt5WebEngineWidgets_FOUND)

################# compiles resources files ############
SET(ONLINEQUERIES_RES ../resources/OnlineQueries.qrc)
QT5_ADD_RESOURCES(ONLINEQUERIES_RES_CXX ${ONLINEQUERIES_RES})

ADD_LIBRARY(OnlineQueries-static STATIC ${ONLINEQUERIES_SRCS} ${ONLINEQUERIES_RES_CXX} ${ONLINEQUERIES_UIS_H})
TARGET_LINK_LIBRARIES(OnlineQueries-static Qt5::Core Qt5::Network Qt5::Widgets ${QTWEBENGINE_LIBS})
SET_TARGET_PROPERTIES(OnlineQueries-static PROPERTIES OUTPUT_NAME "OnlineQueries")
SET_TARGET_PROPERTIES(OnlineQueries-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
ADD_DEPENDENCIES(AllStaticPlugins OnlineQueries-static)

SET_TARGET_PROPERTIES(OnlineQueries-static PROPERTIES FOLDER "plugins/OnlineQueries")

