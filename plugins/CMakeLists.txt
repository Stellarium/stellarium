IF(WIN32)
  SET(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} "-Wl,--enable-runtime-pseudo-reloc")
ENDIF()

ADD_DEFINITIONS(-DQT_PLUGIN)
ADD_DEFINITIONS(-DQT_DLL)
ADD_DEFINITIONS(-D_REENTRANT)

FOREACH(NAME ${STELLARIUM_PLUGINS})
  STRING(TOUPPER ${NAME} NAME_UP)
  IF(USE_PLUGIN_${NAME_UP})
    ADD_SUBDIRECTORY(${NAME})
  ENDIF()
ENDFOREACH()

IF(USE_PLUGIN_SVMT)
  SET(SOURCE_SVMT_PATH "../../svmt/" CACHE PATH "The location of the SVMT plugin main directory i.e. the one containing the top level CMakeLists.txt")
  ADD_SUBDIRECTORY( ${SOURCE_SVMT_PATH} svmt )
ENDIF()

IF(USE_PLUGIN_VIRGO)
  SET(VIRGO_PROJECT_PATH "../extmodules/VirGO/" CACHE PATH "The location of the VirGO plugin main directory i.e. the one containing the top level CMakeLists.txt")
  ADD_SUBDIRECTORY( ${VIRGO_PROJECT_PATH} VirGO )
ENDIF()

