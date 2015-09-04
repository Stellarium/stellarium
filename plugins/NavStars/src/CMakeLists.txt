INCLUDE_DIRECTORIES(
    .
    ${CMAKE_BINARY_DIR}/plugins/NavStars/src
)

LINK_DIRECTORIES(${BUILD_DIR}/src)

SET(NavStars_SRCS
  NavStars.hpp
  NavStars.cpp
)

SET(NavStars_RES ../resources/NavStars.qrc)
QT5_ADD_RESOURCES(NavStars_RES_CXX ${NavStars_RES})

SET(extLinkerOption ${PNG_LIBRARIES} ${OPENGL_LIBRARIES})

ADD_LIBRARY(NavStars-static STATIC ${NavStars_SRCS} ${NavStars_RES_CXX})
QT5_USE_MODULES(NavStars-static Core Network Widgets)
SET_TARGET_PROPERTIES(NavStars-static PROPERTIES OUTPUT_NAME "NavStars")
TARGET_LINK_LIBRARIES(NavStars-static ${extLinkerOption})
SET_TARGET_PROPERTIES(NavStars-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
ADD_DEPENDENCIES(AllStaticPlugins NavStars-static)
