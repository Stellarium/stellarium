set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

LINK_DIRECTORIES(/src)

#QT5_WRAP_UI(TelescopeControl_UIS_H ${TelescopeControl_UIS})

# Compile the resource files
#SET(TelescopeControl_RES ../resources/TelescopeControl.qrc)
#QT5_ADD_RESOURCES(TelescopeControl_RES_CXX ${TelescopeControl_RES})

#IF(WIN32 AND NOT MSVC)
# TODO: Test if winsock is still necessary if the Telescope Control plug-in is disabled.
# Because including it here seems to duplicate STELLARIUM_STATIC_PLUGINS_LIBRARIES
#   SET(extLinkerOption ${extLinkerOption} libwinmm.a -lws2_32)
#ENDIF()
#

add_subdirectory(common)
add_subdirectory(gui)
add_subdirectory(Lx200)
add_subdirectory(NexStar)
add_subdirectory(Rts2)
add_subdirectory(INDI)

ADD_LIBRARY(TelescopeControl-static STATIC
    TelescopeControlGlobals.hpp
    TelescopeControl.hpp
    TelescopeControl.cpp
    TelescopeClient.hpp
    TelescopeClient.cpp
    ../resources/TelescopeControl.qrc
    )

SET_TARGET_PROPERTIES(TelescopeControl-static PROPERTIES OUTPUT_NAME "TelescopeControl")

TARGET_LINK_LIBRARIES(TelescopeControl-static
    Qt5::Core
    Qt5::Widgets
    Qt5::SerialPort
    TelescopeControl_gui
    TelescopeControl_Lx200
    TelescopeControl_INDI
    TelescopeControl_NexStar
    TelescopeControl_Rts2
    TelescopeControl_common
    )
SET_TARGET_PROPERTIES(TelescopeControl-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
ADD_DEPENDENCIES(AllStaticPlugins TelescopeControl-static)

