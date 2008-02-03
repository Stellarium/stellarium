# PYQT4_WRAP_UI(outfiles inputfile ... )
MACRO (PYQT4_WRAP_UI outfiles )
  FOREACH (it ${ARGN})
    GET_FILENAME_COMPONENT(outfile ${it} NAME_WE)
    GET_FILENAME_COMPONENT(infile ${it} ABSOLUTE)
    SET(outfile ${CMAKE_CURRENT_BINARY_DIR}/ui_${outfile}.py)
    ADD_CUSTOM_COMMAND(OUTPUT ${outfile}
      COMMAND ${QT_PYUIC_EXECUTABLE}
      ARGS -o ${outfile} ${infile}
      MAIN_DEPENDENCY ${infile})
    SET(${outfiles} ${${outfiles}} ${outfile})
  ENDFOREACH (it)
ENDMACRO (PYQT4_WRAP_UI)

MACRO (PYQT4_ADD_RESOURCES outfiles )
	FOREACH (it ${ARGN})
	GET_FILENAME_COMPONENT(outfilename ${it} NAME_WE)
	GET_FILENAME_COMPONENT(infile ${it} ABSOLUTE)
	GET_FILENAME_COMPONENT(rc_path ${infile} PATH)
	SET(outfile ${CMAKE_CURRENT_BINARY_DIR}/${outfilename}_rc.py)
#  parse file for dependencies 
#  all files are absolute paths or relative to the location of the qrc file
	FILE(READ "${infile}" _RC_FILE_CONTENTS)
	STRING(REGEX MATCHALL "<file[^<]+" _RC_FILES "${_RC_FILE_CONTENTS}")
	SET(_RC_DEPENDS)
	FOREACH(_RC_FILE ${_RC_FILES})
		STRING(REGEX REPLACE "^<file[^>]*>" "" _RC_FILE "${_RC_FILE}")
		STRING(REGEX MATCH "^/|([A-Za-z]:/)" _ABS_PATH_INDICATOR "${_RC_FILE}")
		IF(NOT _ABS_PATH_INDICATOR)
		SET(_RC_FILE "${rc_path}/${_RC_FILE}")
		ENDIF(NOT _ABS_PATH_INDICATOR)
		SET(_RC_DEPENDS ${_RC_DEPENDS} "${_RC_FILE}")
	ENDFOREACH(_RC_FILE)
	ADD_CUSTOM_COMMAND(OUTPUT ${outfile}
		COMMAND ${PYQT_RCC_EXECUTABLE}
		ARGS -name ${outfilename} -o ${outfile} ${infile}
		MAIN_DEPENDENCY ${infile}
		DEPENDS ${_RC_DEPENDS})
	SET(${outfiles} ${${outfiles}} ${outfile})
	ENDFOREACH (it)
ENDMACRO (PYQT4_ADD_RESOURCES)

SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/sip)
FIND_PACKAGE(PythonLibrary REQUIRED)
FIND_PACKAGE(SIP REQUIRED)
FIND_PACKAGE(PyQt4 REQUIRED)
INCLUDE(SIPMacros.cmake)

SET(SIP_INCLUDES /usr/share/sip/PyQt4)
SET(SIP_TAGS WS_X11 Qt_4_3_0)
SET(SIP_CONCAT_PARTS 1)
SET(SIP_EXTRA_OPTIONS -w)

INCLUDE_DIRECTORIES( ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/core ${CMAKE_SOURCE_DIR}/src/deprecated ${CMAKE_SOURCE_DIR}/src/modules ${CMAKE_SOURCE_DIR}/src/planetsephems ${CMAKE_SOURCE_DIR}/src/stelutils ${FreeType2_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR} ${CURL_INCLUDE_DIRS} ${JPEG_INCLUDE_DIR} ${Boost_INCLUDE_DIRS} ${SDL_INCLUDE_DIR} ${ICONV_INCLUDE_DIR} ${INTL_INCLUDE_DIR} ${SDLMIXER_INCLUDE_DIR} ${QT_INCLUDE_DIR}/QtScript ${QT_QTSCRIPT_INCLUDE_DIR} ${SIP_INCLUDE_DIR} )

ADD_SIP_PYTHON_MODULE(PyStellarium PyStellarium.sip stelMain)

SET(QT_PYUIC_EXECUTABLE /usr/bin/pyuic4)
PYQT4_WRAP_UI(stellarium_UIS_PY ${CMAKE_SOURCE_DIR}/src/mainGui.ui)
ADD_CUSTOM_TARGET(BuildPythonUI ALL DEPENDS ${stellarium_UIS_PY} COMMENT "Building python UI")

SET(PYQT_RCC_EXECUTABLE /usr/bin/pyrcc4)
PYQT4_ADD_RESOURCES(stellarium_RES_PY ${CMAKE_SOURCE_DIR}/data/mainRes.qrc)
ADD_CUSTOM_TARGET(BuildPythonRes ALL DEPENDS ${stellarium_RES_PY} COMMENT "Building python RES")