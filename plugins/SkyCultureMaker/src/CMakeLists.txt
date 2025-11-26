INCLUDE_DIRECTORIES(
     .
     gui
     ${CMAKE_BINARY_DIR}/plugins/SkyCultureMaker/src
     ${CMAKE_BINARY_DIR}/plugins/SkyCultureMaker/src/gui
)

LINK_DIRECTORIES(${CMAKE_BINARY_DIR}/src)

SET( SkyCultureMaker_SRCS

     enumBitops.hpp
     ScmConstellation.hpp
     ScmConstellation.cpp
     ScmConstellationArtwork.hpp
     ScmConstellationArtwork.cpp
     ScmDraw.hpp
     ScmDraw.cpp
     ScmSkyCulture.hpp
     ScmSkyCulture.cpp
     SkyCultureMaker.hpp
     SkyCultureMaker.cpp

     gui/ScmConstellationDialog.hpp
     gui/ScmConstellationDialog.cpp
     gui/ScmConstellationImage.hpp
     gui/ScmConstellationImage.cpp
     gui/ScmConstellationImageAnchor.hpp
     gui/ScmConstellationImageAnchor.cpp
     gui/ScmHideOrAbortMakerDialog.hpp
     gui/ScmHideOrAbortMakerDialog.cpp
     gui/ScmSkyCultureDialog.hpp
     gui/ScmSkyCultureDialog.cpp
     gui/ScmSkyCultureExportDialog.hpp
     gui/ScmSkyCultureExportDialog.cpp
     gui/ScmStartDialog.hpp
     gui/ScmStartDialog.cpp

     types/Anchor.hpp
     types/Classification.hpp
     types/ConstellationLine.hpp
     types/Description.hpp
     types/DialogID.hpp
     types/Drawing.hpp
     types/DrawingMode.hpp
     types/DrawTools.hpp
     types/License.hpp
     types/SkyPoint.hpp
)

SET( SCM_UIS
     gui/scmConstellationDialog.ui
     gui/scmHideOrAbortMakerDialog.ui
     gui/scmSkyCultureDialog.ui
     gui/scmSkyCultureExportDialog.ui
     gui/scmStartDialog.ui
)

if(SCM_CONVERTER_ENABLED)
  list(APPEND SkyCultureMaker_SRCS
    gui/ScmConvertDialog.hpp
    gui/ScmConvertDialog.cpp
  )
  list(APPEND SCM_UIS
    gui/scmConvertDialog.ui
  )
endif()

################# compiles resources files ############
SET(SkyCultureMaker_RES ../resources/SkyCultureMaker.qrc)
IF (${QT_VERSION_MAJOR} EQUAL "5")
     QT5_WRAP_UI(SCM_UIS_H ${SCM_UIS})
     QT5_ADD_RESOURCES(SkyCultureMaker_RES_CXX ${SkyCultureMaker_RES})
ELSE()
     QT_WRAP_UI(SCM_UIS_H ${SCM_UIS})
     QT_ADD_RESOURCES(SkyCultureMaker_RES_CXX ${SkyCultureMaker_RES})
ENDIF()

ADD_LIBRARY(SkyCultureMaker-static STATIC ${SkyCultureMaker_SRCS} ${SkyCultureMaker_RES_CXX} ${SCM_UIS})
SET_TARGET_PROPERTIES(SkyCultureMaker-static PROPERTIES OUTPUT_NAME "SkyCultureMaker")

list(APPEND SKY_CULTURE_MAKER_PLUGIN_LINK_LIBS Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets)
# SCM_CONVERTER_ENABLED is inherited from the parent CMakeLists.txt scope
if(SCM_CONVERTER_ENABLED)
  list(APPEND SKY_CULTURE_MAKER_PLUGIN_LINK_LIBS libskycultureconverter unarr)
endif()

TARGET_LINK_LIBRARIES(SkyCultureMaker-static ${SKY_CULTURE_MAKER_PLUGIN_LINK_LIBS})
SET_TARGET_PROPERTIES(SkyCultureMaker-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
ADD_DEPENDENCIES(AllStaticPlugins SkyCultureMaker-static)

SET_TARGET_PROPERTIES(SkyCultureMaker-static PROPERTIES FOLDER "plugins/SkyCultureMaker")
