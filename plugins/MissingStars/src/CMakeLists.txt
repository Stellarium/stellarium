INCLUDE_DIRECTORIES(
     .
     gui
     ${CMAKE_BINARY_DIR}/plugins/MissingStars/src
     ${CMAKE_BINARY_DIR}/plugins/MissingStars/src/gui
)

LINK_DIRECTORIES(${CMAKE_BINARY_DIR}/src)

SET(MissingStars_SRCS
     MissingStars.hpp
     MissingStars.cpp
     MissingStar.hpp
     MissingStar.cpp
     gui/MissingStarsDialog.hpp
     gui/MissingStarsDialog.cpp
)

SET(MissingStarsDialog_UIS
     gui/missingStarsDialog.ui
)

SET(MissingStars_RES ../resources/MissingStars.qrc)
IF (${QT_VERSION_MAJOR} EQUAL "5")
     QT5_WRAP_UI(MissingStarsDialog_UIS_H ${MissingStarsDialog_UIS})
     QT5_ADD_RESOURCES(MissingStars_RES_CXX ${MissingStars_RES})
ELSE()
     QT_WRAP_UI(MissingStarsDialog_UIS_H ${MissingStarsDialog_UIS})
     QT_ADD_RESOURCES(MissingStars_RES_CXX ${MissingStars_RES})
ENDIF()

ADD_LIBRARY(MissingStars-static STATIC ${MissingStars_SRCS} ${MissingStars_RES_CXX} ${MissingStarsDialog_UIS_H})
TARGET_LINK_LIBRARIES(MissingStars-static Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets)

SET_TARGET_PROPERTIES(MissingStars-static PROPERTIES OUTPUT_NAME "MissingStars")
SET_TARGET_PROPERTIES(MissingStars-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
ADD_DEPENDENCIES(AllStaticPlugins MissingStars-static)

SET_TARGET_PROPERTIES(MissingStars-static PROPERTIES FOLDER "plugins/MissingStars")

