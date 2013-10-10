INCLUDE_DIRECTORIES(.)
LINK_DIRECTORIES(${BUILD_DIR}/src)

SET(AngleMeasure_SRCS
  AngleMeasure.hpp
  AngleMeasure.cpp)

################# compiles resources files ############
SET(AngleMeasure_RES ../AngleMeasure.qrc)
QT5_ADD_RESOURCES(AngleMeasure_RES_CXX ${AngleMeasure_RES})
# 
SET(extLinkerOption ${FreeType2_LIBRARIES} ${JPEG_LIBRARIES} ${PNG_LIBRARIES} ${OPENGL_LIBRARIES})

############### For building the static library ######################
IF(BUILD_STATIC_PLUGINS)
	ADD_LIBRARY(AngleMeasure-static STATIC ${AngleMeasure_SRCS} ${AngleMeasure_RES_CXX})
	QT5_USE_MODULES(AngleMeasure-static Core Network Widgets)
	# The library target "AngleMeasure-static" has a default OUTPUT_NAME of "AngleMeasure-static", so change it.
	SET_TARGET_PROPERTIES(AngleMeasure-static PROPERTIES OUTPUT_NAME "AngleMeasure")
	TARGET_LINK_LIBRARIES(AngleMeasure-static ${extLinkerOption})
	SET_TARGET_PROPERTIES(AngleMeasure-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
	ADD_DEPENDENCIES(AllStaticPlugins AngleMeasure-static)
ENDIF()

