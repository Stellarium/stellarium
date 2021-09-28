INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(common)
add_subdirectory(gui)
add_subdirectory(Lx200)
add_subdirectory(NexStar)
add_subdirectory(Rts2)
add_subdirectory(INDI)
if(WIN32)
    add_subdirectory(ASCOM)
endif(WIN32)

SET(TelescopeControl_RES ../resources/TelescopeControl.qrc)
QT5_ADD_RESOURCES(TelescopeControl_RES_CXX ${TelescopeControl_RES})

ADD_LIBRARY(TelescopeControl-static STATIC
    TelescopeControlGlobals.hpp
    TelescopeControl.hpp
    TelescopeControl.cpp
    TelescopeClient.hpp
    TelescopeClient.cpp
    ${TelescopeControl_RES_CXX}
    )

SET_TARGET_PROPERTIES(TelescopeControl-static PROPERTIES OUTPUT_NAME "TelescopeControl")

TARGET_LINK_LIBRARIES(TelescopeControl-static
    TelescopeControl_gui
    TelescopeControl_Lx200
    TelescopeControl_INDI
    TelescopeControl_NexStar
    TelescopeControl_Rts2
    TelescopeControl_common
    Qt5::Core
    Qt5::Widgets
    Qt5::SerialPort
    )

IF(WIN32)
TARGET_LINK_LIBRARIES(TelescopeControl-static
    TelescopeControl_ASCOM)
ENDIF(WIN32)

SET_TARGET_PROPERTIES(TelescopeControl-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
ADD_DEPENDENCIES(AllStaticPlugins TelescopeControl-static)

SET_TARGET_PROPERTIES(TelescopeControl-static PROPERTIES FOLDER "plugins/TelescopeControl")
