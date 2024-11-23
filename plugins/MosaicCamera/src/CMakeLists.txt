INCLUDE_DIRECTORIES(.)

LINK_DIRECTORIES(${BUILD_DIR}/src)

SET(MosaicCamera_SRCS
     MosaicCamera.hpp
     MosaicCamera.cpp
)

SET(MosaicCamera_Qt_Libraries Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Gui)

############### For building the dynamic library ######################
IF(BUILD_DYNAMIC_PLUGIN)
     ADD_LIBRARY(MosaicCamera MODULE ${MosaicCamera_SRCS})
     IF(APPLE)
          FIND_LIBRARY(OPENGL_LIBRARY OpenGL)
          MARK_AS_ADVANCED(OPENGL_LIBRARY)
          SET_TARGET_PROPERTIES(MosaicCamera PROPERTIES LINK_FLAGS "-undefined dynamic_lookup" SUFFIX ".dylib")
     ENDIF()

     IF(WIN32)
          SET_TARGET_PROPERTIES(MosaicCamera PROPERTIES LINK_FLAGS "-enable-runtime-pseudo-reloc -Wl,--allow-multiple-definition" )
          SET(StelMain stelMain)
     ELSE(WIN32)
          SET(StelMain )
     ENDIF(WIN32)

     TARGET_LINK_LIBRARIES(MosaicCamera ${StelMain} ${MosaicCamera_Qt_Libraries})
     INSTALL(TARGETS MosaicCamera DESTINATION "modules/MosaicCamera")
ELSE()
############### For building the static library ######################
     ADD_LIBRARY(MosaicCamera-static STATIC ${MosaicCamera_SRCS})
     # The library target "MosaicCamera-static" has a default OUTPUT_NAME of "MosaicCamera-static", so change it.
     SET_TARGET_PROPERTIES(MosaicCamera-static PROPERTIES OUTPUT_NAME "MosaicCamera")
     TARGET_LINK_LIBRARIES(MosaicCamera-static ${StelMain} ${MosaicCamera_Qt_Libraries})
     SET_TARGET_PROPERTIES(MosaicCamera-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
     ADD_DEPENDENCIES(AllStaticPlugins MosaicCamera-static)
ENDIF()

SET_TARGET_PROPERTIES(MosaicCamera-static PROPERTIES FOLDER "plugins/MosaicCamera")
