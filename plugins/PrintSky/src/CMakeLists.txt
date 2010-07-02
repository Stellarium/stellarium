INCLUDE_DIRECTORIES(
	.
	gui
	${CMAKE_BINARY_DIR}/plugins/PrintSky/src
)
LINK_DIRECTORIES(${BUILD_DIR}/src)

SET(PrintSky_SRCS
  PrintSky.hpp
  PrintSky.cpp
  gui/PrintSkyDialog.hpp
  gui/PrintSkyDialog.cpp
  gui/StelDialogPrintSky.hpp
  gui/StelDialogPrintSky.cpp
  )

################# compiles resources files ############
SET(PrintSky_RES ../PrintSky.qrc)
QT4_ADD_RESOURCES(PrintSky_RES_CXX ${PrintSky_RES})

################# compiles .ui files ############
SET(PrintSky_UIS
  gui/printskyDialog.ui
)
QT4_WRAP_UI(PrintSky_UIS_H ${PrintSky_UIS})

# Add here all the files which require moc compilation
# that is, all the headers with SIGNAL/SLOTS/PLUGIN_INTERFACE etc..
SET(PrintSky_MOC_HDRS
 PrintSky.hpp
 gui/PrintSkyDialog.hpp
 gui/StelDialogPrintSky.hpp
 )

# After this call, PrintSky_MOC_SRCS = moc_PrintSky.cxx
QT4_WRAP_CPP(PrintSky_MOC_SRCS ${PrintSky_MOC_HDRS})

SET(extLinkerOption ${QT_LIBRARIES} ${FreeType2_LIBRARIES} ${JPEG_LIBRARIES} ${PNG_LIBRARIES} ${OPENGL_LIBRARIES} ${ICONV_LIBRARIES} ${INTL_LIBRARIES})

############### For building the dynamic library ######################
IF(BUILD_DYNAMIC_PLUGINS)
	ADD_LIBRARY(PrintSky MODULE ${PrintSky_SRCS} ${PrintSky_MOC_SRCS} ${PrintSky_RES_CXX} ${PrintSky_UIS_H})
	IF(APPLE)
		FIND_LIBRARY(OPENGL_LIBRARY OpenGL)
		MARK_AS_ADVANCED(OPENGL_LIBRARY)
		SET_TARGET_PROPERTIES(PrintSky PROPERTIES LINK_FLAGS "-undefined dynamic_lookup" SUFFIX ".dylib")
	ENDIF(APPLE)

	IF(WIN32)
		SET_TARGET_PROPERTIES(PrintSky PROPERTIES LINK_FLAGS "-enable-runtime-pseudo-reloc -Wl,--allow-multiple-definition" )
		SET(StelMain stelMain)
	ELSE(WIN32)
		SET(StelMain )
	ENDIF(WIN32)

	TARGET_LINK_LIBRARIES(PrintSky ${StelMain} ${extLinkerOption})
	INSTALL(TARGETS PrintSky DESTINATION "modules/${PACKAGE}")
ENDIF()

############### For building the static library ######################
IF(BUILD_STATIC_PLUGINS)
	ADD_LIBRARY(PrintSky-static STATIC ${PrintSky_SRCS} ${PrintSky_MOC_SRCS} ${PrintSky_RES_CXX} ${PrintSky_UIS_H})
	# The library target "PrintSky-static" has a default OUTPUT_NAME of "PrintSky-static", so change it.
	SET_TARGET_PROPERTIES(PrintSky-static PROPERTIES OUTPUT_NAME "PrintSky")
	TARGET_LINK_LIBRARIES(PrintSky-static ${extLinkerOption})
	SET_TARGET_PROPERTIES(PrintSky-static PROPERTIES COMPILE_FLAGS "-fPIC -DQT_STATICPLUGIN")
	ADD_DEPENDENCIES(AllStaticPlugins PrintSky-static)
ENDIF()

