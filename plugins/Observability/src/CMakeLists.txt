INCLUDE_DIRECTORIES(
     .
     gui
     ${CMAKE_BINARY_DIR}/plugins/Observability/src
     ${CMAKE_BINARY_DIR}/plugins/Observability/src/gui
)

LINK_DIRECTORIES(${CMAKE_BINARY_DIR}/src)

SET(Observability_SRCS
     Observability.hpp
     Observability.cpp
     gui/ObservabilityDialog.hpp
     gui/ObservabilityDialog.cpp
)

################# compiles resources files ############

SET(ObservabilityDialog_UIS
     gui/ObservabilityDialog.ui
)
QT_WRAP_UI(ObservabilityDialog_UIS_H ${ObservabilityDialog_UIS})


SET(Observability_RES ../Observability.qrc)
QT_ADD_RESOURCES(Observability_RES_CXX ${Observability_RES})

ADD_LIBRARY(Observability-static STATIC ${Observability_SRCS} ${Observability_RES_CXX} ${ObservabilityDialog_UIS_H})
TARGET_LINK_LIBRARIES(Observability-static Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets)
SET_TARGET_PROPERTIES(Observability-static PROPERTIES OUTPUT_NAME "Observability")
SET_TARGET_PROPERTIES(Observability-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
ADD_DEPENDENCIES(AllStaticPlugins Observability-static)

SET_TARGET_PROPERTIES(Observability-static PROPERTIES FOLDER "plugins/Observability")
