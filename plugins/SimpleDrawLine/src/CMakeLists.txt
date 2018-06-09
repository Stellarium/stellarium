INCLUDE_DIRECTORIES(.)

LINK_DIRECTORIES(${BUILD_DIR}/src)

SET(SimpleDrawLine_SRCS
     SimpleDrawLine.hpp
     SimpleDrawLine.cpp
)

SET(SimpleDrawLine_Qt_Libraries Qt5::Core Qt5::Gui)

############### For building the dynamic library ######################
IF(BUILD_DYNAMIC_PLUGIN)
     ADD_LIBRARY(SimpleDrawLine MODULE ${SimpleDrawLine_SRCS})
     IF(APPLE)
          FIND_LIBRARY(OPENGL_LIBRARY OpenGL)
          MARK_AS_ADVANCED(OPENGL_LIBRARY)
          SET_TARGET_PROPERTIES(SimpleDrawLine PROPERTIES LINK_FLAGS "-undefined dynamic_lookup" SUFFIX ".dylib")
     ENDIF()

     IF(WIN32)
          SET_TARGET_PROPERTIES(SimpleDrawLine PROPERTIES LINK_FLAGS "-enable-runtime-pseudo-reloc -Wl,--allow-multiple-definition" )
          SET(StelMain stelMain)
     ELSE(WIN32)
          SET(StelMain )
     ENDIF(WIN32)

     TARGET_LINK_LIBRARIES(SimpleDrawLine ${StelMain} ${SimpleDrawLine_Qt_Libraries})
     INSTALL(TARGETS SimpleDrawLine DESTINATION "modules/SimpleDrawLine")
ELSE()
############### For building the static library ######################
     ADD_LIBRARY(SimpleDrawLine-static STATIC ${SimpleDrawLine_SRCS})
     # The library target "SimpleDrawLine-static" has a default OUTPUT_NAME of "SimpleDrawLine-static", so change it.
     SET_TARGET_PROPERTIES(SimpleDrawLine-static PROPERTIES OUTPUT_NAME "SimpleDrawLine")
     TARGET_LINK_LIBRARIES(SimpleDrawLine-static ${StelMain} ${SimpleDrawLine_Qt_Libraries})
     SET_TARGET_PROPERTIES(SimpleDrawLine-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
     ADD_DEPENDENCIES(AllStaticPlugins SimpleDrawLine-static)
ENDIF()

SET_TARGET_PROPERTIES(SimpleDrawLine-static PROPERTIES FOLDER "plugins/SimpleDrawLine")
