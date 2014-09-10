INCLUDE_DIRECTORIES(
    .
    gui
    ${CMAKE_BINARY_DIR}/plugins/FOV/src
    ${CMAKE_BINARY_DIR}/plugins/FOV/src/gui
)

LINK_DIRECTORIES(${BUILD_DIR}/src)

SET(FOV_SRCS
  FOV.hpp
  FOV.cpp
  gui/FOVWindow.hpp
  gui/FOVWindow.cpp
)

SET(FOV_UIS
  gui/fovWindow.ui
)
QT4_WRAP_UI(FOV_UIS_H ${FOV_UIS})

SET(extLinkerOption ${QT_LIBRARIES} ${ICONV_LIBRARIES} ${INTL_LIBRARIES})

############### For building the dynamic library ######################
IF(BUILD_DYNAMIC_PLUGINS)
	ADD_LIBRARY(FOV MODULE ${FOV_SRCS} ${FOV_UIS_H})
	IF(APPLE)		
		SET_TARGET_PROPERTIES(FOV PROPERTIES LINK_FLAGS "-undefined dynamic_lookup" SUFFIX ".dylib")
	ENDIF()

	IF(WIN32)
                SET_TARGET_PROPERTIES(FOV PROPERTIES LINK_FLAGS "-Wl,--enable-runtime-pseudo-reloc -Wl,--allow-multiple-definition" )
		SET(StelMain stelMain)
	ELSE(WIN32)
		SET(StelMain )
	ENDIF(WIN32)

	TARGET_LINK_LIBRARIES(FOV ${StelMain} ${extLinkerOption})
	INSTALL(TARGETS FOV DESTINATION "modules/FOV")
ENDIF()

############### For building the static library ######################
IF(BUILD_STATIC_PLUGINS)
	ADD_LIBRARY(FOV-static STATIC ${FOV_SRCS} ${FOV_UIS_H})
	SET_TARGET_PROPERTIES(FOV-static PROPERTIES OUTPUT_NAME "FOV")
	TARGET_LINK_LIBRARIES(FOV-static ${extLinkerOption})
	SET_TARGET_PROPERTIES(FOV-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
	ADD_DEPENDENCIES(AllStaticPlugins FOV-static)
ENDIF()
