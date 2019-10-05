if(WIN32)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

find_package(Threads REQUIRED)

SET(TelescopeControl_ASCOM_UIS
    TelescopeClientASCOMWidget.ui
    )

QT5_WRAP_UI(TelescopeControl_ASCOM_UIS_H ${TelescopeControl_ASCOM_UIS})

add_library(TelescopeControl_ASCOM STATIC
    ASCOMDevice.hpp
    ASCOMDevice.cpp
    TelescopeClientASCOM.hpp
    TelescopeClientASCOM.cpp
    TelescopeClientASCOMWidget.hpp
    TelescopeClientASCOMWidget.cpp
    ${TelescopeControl_ASCOM_UIS_H}
    )

target_link_libraries(TelescopeControl_ASCOM
    Qt5::Core
    Qt5::Network
    TelescopeControl_common
    ${CMAKE_THREAD_LIBS_INIT}
    )

target_include_directories(TelescopeControl_ASCOM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

IF(ENABLE_TESTING)
    add_subdirectory(test)
ENDIF(ENABLE_TESTING)

SET_TARGET_PROPERTIES(TelescopeControl_ASCOM PROPERTIES FOLDER "plugins/TelescopeControl")

ENDIF(WIN32)