set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

find_package(Threads REQUIRED)

SET(TelescopeControl_Alpaca_UIS
    TelescopeClientAlpacaWidget.ui
    )

IF (${QT_VERSION_MAJOR} EQUAL "5")
     QT5_WRAP_UI(TelescopeControl_Alpaca_UIS_H ${TelescopeControl_Alpaca_UIS})
ELSE(${QT_VERSION_MAJOR} EQUAL "5")
     QT_WRAP_UI(TelescopeControl_Alpaca_UIS_H ${TelescopeControl_Alpaca_UIS})
ENDIF(${QT_VERSION_MAJOR} EQUAL "5")

add_library(TelescopeControl_Alpaca STATIC
    AlpacaDevice.hpp
    AlpacaDevice.cpp
    TelescopeClientAlpaca.hpp
    TelescopeClientAlpaca.cpp
    TelescopeClientAlpacaWidget.hpp
    TelescopeClientAlpacaWidget.cpp
    ${TelescopeControl_Alpaca_UIS_H}
    )

target_link_libraries(TelescopeControl_Alpaca
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Network
    TelescopeControl_common
    ${CMAKE_THREAD_LIBS_INIT}
    )

target_include_directories(TelescopeControl_Alpaca INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

IF(ENABLE_TESTING)
    add_subdirectory(test)
ENDIF(ENABLE_TESTING)

SET_TARGET_PROPERTIES(TelescopeControl_Alpaca PROPERTIES FOLDER "plugins/TelescopeControl")

