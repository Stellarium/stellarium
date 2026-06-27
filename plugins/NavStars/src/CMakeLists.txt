INCLUDE_DIRECTORIES(
     .
     gui
     ${CMAKE_BINARY_DIR}/plugins/NavStars/src
     ${CMAKE_BINARY_DIR}/plugins/NavStars/src/gui
)

LINK_DIRECTORIES(${CMAKE_BINARY_DIR}/src)

SET(NavStars_SRCS
     NavStars.hpp
     NavStars.cpp
     NavStarsCalculator.hpp
     NavStarsCalculator.cpp
)
IF (STELLARIUM_GUI_MODE STREQUAL "Standard")
LIST(APPEND NavStars_SRCS
     gui/NavStarsWindow.hpp
     gui/NavStarsWindow.cpp
)

SET(NavStars_UIS
     gui/navStarsWindow.ui
)
ENDIF (STELLARIUM_GUI_MODE STREQUAL "Standard")

SET(NavStars_RES ../resources/NavStars.qrc)
IF (${QT_VERSION_MAJOR} EQUAL "5")
     IF (STELLARIUM_GUI_MODE STREQUAL "Standard")
          QT5_WRAP_UI(NavStars_UIS_H ${NavStars_UIS})
     ENDIF()
     QT5_ADD_RESOURCES(NavStars_RES_CXX ${NavStars_RES})
ELSE()
     IF (STELLARIUM_GUI_MODE STREQUAL "Standard")
          QT_WRAP_UI(NavStars_UIS_H ${NavStars_UIS})
     ENDIF()
     QT_ADD_RESOURCES(NavStars_RES_CXX ${NavStars_RES})
ENDIF()

ADD_LIBRARY(NavStars-static STATIC ${NavStars_SRCS} ${NavStars_RES_CXX} ${NavStars_UIS_H})
TARGET_LINK_LIBRARIES(NavStars-static Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::OpenGL)

IF(ENABLE_TESTING)
    add_definitions("-DWITH_TESTING_ON")
    ADD_SUBDIRECTORY(test)
ENDIF(ENABLE_TESTING)

SET_TARGET_PROPERTIES(NavStars-static PROPERTIES OUTPUT_NAME "NavStars")
SET_TARGET_PROPERTIES(NavStars-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
ADD_DEPENDENCIES(AllStaticPlugins NavStars-static)

SET_TARGET_PROPERTIES(NavStars-static PROPERTIES FOLDER "plugins/NavStars")
