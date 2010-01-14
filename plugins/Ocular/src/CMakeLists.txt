
INCLUDE_DIRECTORIES(
  ${CMAKE_BINARY_DIR}/src
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/src 
  ${CMAKE_SOURCE_DIR}/src/gui
  ${STELLARIUM_BUILD_DIR}
  ${STELLARIUM_SOURCE_DIR}
  ${STELLARIUM_SOURCE_DIR}/core
  ${STELLARIUM_SOURCE_DIR}/gui
  ${STELLARIUM_SOURCE_DIR}/modules
  ${STELLARIUM_SOURCE_DIR}/stelutils
  ${STELLARIUM_SOURCE_DIR}/external
  ${STELLARIUM_SOURCE_DIR}/scripting
  ${OPENGL_INCLUDE_DIR}
)

LINK_DIRECTORIES(${STELLARIUM_BUILD_DIR}/src)

SET(Oculars_SRCS
  Ocular.hpp
  Ocular.cpp
  Oculars.hpp
  Oculars.cpp
  Telescope.hpp
  Telescope.cpp
  gui/OcularDialog.hpp
  gui/OcularDialog.cpp
  gui/StelDialogOculars.hpp
  gui/StelDialogOculars.cpp
)

SET(Oculars_RES
  ${CMAKE_SOURCE_DIR}/Oculars.qrc)
QT4_ADD_RESOURCES(Oculars_RES_CXX ${Oculars_RES})

################# compiles .ui files ############
SET(Oculars_UIS
  gui/ocularDialog.ui
)
QT4_WRAP_UI(Oculars_UIS_H ${Oculars_UIS})

# Add here all the files which require moc compilation
# that is, all the headers with SIGNAL/SLOTS/PLUGIN_INTERFACE etc..
SET(Oculars_MOC_HDRS
  Ocular.hpp
  Oculars.hpp
  Telescope.hpp
  gui/OcularDialog.hpp
  gui/StelDialogOculars.hpp
)
 
# After this call, Oculars_MOC_SRCS = moc_Oculars.cxx
QT4_WRAP_CPP(Oculars_MOC_SRCS ${Oculars_MOC_HDRS})

# Dummy target depending on ${stellarium_UIS_H} to trigger ui compilations
ADD_CUSTOM_TARGET(GenerateUiHeaders ALL DEPENDS ${Oculars_UIS_H})

SET(extLinkerOption ${QT_LIBRARIES} ${JPEG_LIBRARIES} ${PNG_LIBRARIES} ${OPENGL_LIBRARIES} ${ICONV_LIBRARIES} ${INTL_LIBRARIES})

############### For building the dynamic library ######################
IF(BUILD_DYNAMIC_LIB)
    ADD_LIBRARY(Oculars MODULE ${Oculars_SRCS} ${Oculars_MOC_SRCS} ${Oculars_RES_CXX})

    IF(APPLE)
        FIND_LIBRARY(OPENGL_LIBRARY OpenGL)
        MARK_AS_ADVANCED(OPENGL_LIBRARY)
        SET_TARGET_PROPERTIES(Oculars PROPERTIES LINK_FLAGS "-undefined dynamic_lookup" SUFFIX ".dylib")
    ENDIF(APPLE)

    IF(WIN32)
        SET_TARGET_PROPERTIES(Oculars PROPERTIES LINK_FLAGS "-enable-runtime-pseudo-reloc -Wl,--allow-multiple-definition" )
        SET(StelMain stelMain)
    ELSE(WIN32)
        SET(StelMain )
    ENDIF(WIN32)

    TARGET_LINK_LIBRARIES(Oculars ${StelMain} ${extLinkerOption})
    INSTALL(TARGETS Oculars DESTINATION "modules/${PACKAGE}")
ENDIF(BUILD_DYNAMIC_LIB)

############### For building the static library ######################
IF(BUILD_STATIC_LIB)
    ADD_LIBRARY(Oculars-static STATIC ${Oculars_SRCS} ${Oculars_MOC_SRCS} ${Oculars_RES_CXX})
    SET_TARGET_PROPERTIES(Oculars-static PROPERTIES OUTPUT_NAME "Oculars")
    TARGET_LINK_LIBRARIES(Oculars-static ${StelMain} ${extLinkerOption})
    SET_TARGET_PROPERTIES(Oculars-static PROPERTIES COMPILE_FLAGS "-fPIC -DQT_STATICPLUGIN")
ENDIF(BUILD_STATIC_LIB)

