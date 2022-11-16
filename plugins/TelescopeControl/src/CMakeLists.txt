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
IF (${QT_VERSION_MAJOR} EQUAL "5")
     QT5_ADD_RESOURCES(TelescopeControl_RES_CXX ${TelescopeControl_RES})
ELSE()
     QT_ADD_RESOURCES(TelescopeControl_RES_CXX ${TelescopeControl_RES})
ENDIF()

ADD_LIBRARY(TelescopeControl-static STATIC
    TelescopeControl.hpp
    TelescopeControl.cpp
    TelescopeClient.hpp
    TelescopeClient.cpp
    ${TelescopeControl_RES_CXX}
    )
if(ENABLE_PCH)
    # prepend a few includes to each cpp file.
     target_precompile_headers(TelescopeControl-static PRIVATE
          $<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/TelescopeClient.hpp>
          $<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/TelescopeControl.hpp>)
     IF(CMAKE_VERSION VERSION_GREATER_EQUAL 3.19)
          SET(CMAKE_PCH_INSTANTIATE_TEMPLATES ON)
     ENDIF()
ENDIF(ENABLE_PCH)


SET_TARGET_PROPERTIES(TelescopeControl-static PROPERTIES OUTPUT_NAME "TelescopeControl")

TARGET_LINK_LIBRARIES(TelescopeControl-static
    TelescopeControl_gui
    TelescopeControl_Lx200
    TelescopeControl_INDI
    TelescopeControl_NexStar
    TelescopeControl_Rts2
    TelescopeControl_common
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::SerialPort
    )

IF(WIN32)
TARGET_LINK_LIBRARIES(TelescopeControl-static
    TelescopeControl_ASCOM)
ENDIF(WIN32)

SET_TARGET_PROPERTIES(TelescopeControl-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
ADD_DEPENDENCIES(AllStaticPlugins TelescopeControl-static)

SET_TARGET_PROPERTIES(TelescopeControl-static PROPERTIES FOLDER "plugins/TelescopeControl")
