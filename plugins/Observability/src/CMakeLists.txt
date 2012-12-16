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
QT4_WRAP_UI(ObservabilityDialog_UIS_H ${ObservabilityDialog_UIS})


SET(Observability_RES ../Observability.qrc)
QT4_ADD_RESOURCES(Observability_RES_CXX ${Observability_RES})


SET(extLinkerOption ${QT_LIBRARIES} ${JPEG_LIBRARIES} ${PNG_LIBRARIES} ${OPENGL_LIBRARIES} ${ICONV_LIBRARIES} ${INTL_LIBRARIES})

IF(BUILD_DYNAMIC_PLUGINS)
       ADD_LIBRARY(Observability MODULE ${Observability_SRCS} ${Observability_RES_CXX} ${ObservabilityDialog_UIS_H})
       IF(APPLE)
               FIND_LIBRARY(OPENGL_LIBRARY OpenGL)
               MARK_AS_ADVANCED(OPENGL_LIBRARY)
               SET_TARGET_PROPERTIES(Observability PROPERTIES LINK_FLAGS "-undefined dynamic_lookup" SUFFIX ".dylib")
       ENDIF()
       IF(WIN32)
               SET_TARGET_PROPERTIES(Observability PROPERTIES LINK_FLAGS "-enable-runtime-pseudo-reloc -Wl,--allow-multiple-definition" )
               SET(StelMain stelMain)
       ELSE(WIN32)
               SET(StelMain )
       ENDIF(WIN32)

       TARGET_LINK_LIBRARIES(Observability ${StelMain} ${extLinkerOption})
       INSTALL(TARGETS Observability DESTINATION "modules/Observability")
ENDIF()

IF(BUILD_STATIC_PLUGINS)
       ADD_LIBRARY(Observability-static STATIC ${Observability_SRCS} ${Observability_RES_CXX} ${ObservabilityDialog_UIS_H})
       SET_TARGET_PROPERTIES(Observability-static PROPERTIES OUTPUT_NAME "Observability")
       TARGET_LINK_LIBRARIES(Observability-static ${extLinkerOption})
       SET_TARGET_PROPERTIES(Observability-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
       ADD_DEPENDENCIES(AllStaticPlugins Observability-static)
ENDIF()
