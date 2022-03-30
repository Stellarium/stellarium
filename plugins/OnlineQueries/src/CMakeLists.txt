INCLUDE_DIRECTORIES(
     .
     gui
     ${CMAKE_BINARY_DIR}
     ${CMAKE_BINARY_DIR}/plugins/OnlineQueries/src
     ${CMAKE_BINARY_DIR}/plugins/OnlineQueries/src/gui
)

LINK_DIRECTORIES(${CMAKE_BINARY_DIR}/src)

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

QT_WRAP_UI(ONLINEQUERIES_UIS_H ${ONLINEQUERIES_UIS})

IF(ENABLE_QTWEBENGINE)
    FIND_PACKAGE(Qt${QT_VERSION_MAJOR}WebEngine QUIET)
    FIND_PACKAGE(Qt${QT_VERSION_MAJOR}WebEngineWidgets QUIET)
    IF(Qt${QT_VERSION_MAJOR}WebEngine_FOUND AND Qt${QT_VERSION_MAJOR}WebEngineWidgets_FOUND)
        ADD_DEFINITIONS(-DWITH_QTWEBENGINE)
        SET(QTWEBENGINE_LIBS Qt${QT_VERSION_MAJOR}::WebEngine Qt${QT_VERSION_MAJOR}::WebEngineWidgets)
    ELSE(Qt${QT_VERSION_MAJOR}WebEngine_FOUND AND Qt${QT_VERSION_MAJOR}WebEngineWidgets_FOUND)
        MESSAGE(WARNING "Qt${QT_VERSION_MAJOR}WebEngine and/or Qt${QT_VERSION_MAJOR}WebEngineWidgets not found. \
                         If these packages exist for your platform, install and rerun. \
                         Else the program will call the system web browser where required.")
    ENDIF(Qt${QT_VERSION_MAJOR}WebEngine_FOUND AND Qt${QT_VERSION_MAJOR}WebEngineWidgets_FOUND)
ENDIF(ENABLE_QTWEBENGINE)

################# compiles resources files ############
SET(ONLINEQUERIES_RES ../resources/OnlineQueries.qrc)
QT_ADD_RESOURCES(ONLINEQUERIES_RES_CXX ${ONLINEQUERIES_RES})

ADD_LIBRARY(OnlineQueries-static STATIC ${ONLINEQUERIES_SRCS} ${ONLINEQUERIES_RES_CXX} ${ONLINEQUERIES_UIS_H})
TARGET_LINK_LIBRARIES(OnlineQueries-static Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Network Qt${QT_VERSION_MAJOR}::Widgets ${QTWEBENGINE_LIBS})
SET_TARGET_PROPERTIES(OnlineQueries-static PROPERTIES OUTPUT_NAME "OnlineQueries")
SET_TARGET_PROPERTIES(OnlineQueries-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
ADD_DEPENDENCIES(AllStaticPlugins OnlineQueries-static)

SET_TARGET_PROPERTIES(OnlineQueries-static PROPERTIES FOLDER "plugins/OnlineQueries")

