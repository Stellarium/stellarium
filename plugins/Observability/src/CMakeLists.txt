INCLUDE_DIRECTORIES(
     .
     gui
     ${CMAKE_BINARY_DIR}/plugins/Observability/src
     ${CMAKE_BINARY_DIR}/plugins/Observability/src/gui
)

LINK_DIRECTORIES(${BUILD_DIR}/src)

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
QT5_WRAP_UI(ObservabilityDialog_UIS_H ${ObservabilityDialog_UIS})


SET(Observability_RES ../Observability.qrc)
QT5_ADD_RESOURCES(Observability_RES_CXX ${Observability_RES})

SET(extLinkerOption ${JPEG_LIBRARIES} ${PNG_LIBRARIES} ${OPENGL_LIBRARIES})

ADD_LIBRARY(Observability-static STATIC ${Observability_SRCS} ${Observability_RES_CXX} ${ObservabilityDialog_UIS_H})
QT5_USE_MODULES(Observability-static Core Network Widgets)
SET_TARGET_PROPERTIES(Observability-static PROPERTIES OUTPUT_NAME "Observability")
TARGET_LINK_LIBRARIES(Observability-static ${extLinkerOption})
SET_TARGET_PROPERTIES(Observability-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
ADD_DEPENDENCIES(AllStaticPlugins Observability-static)
