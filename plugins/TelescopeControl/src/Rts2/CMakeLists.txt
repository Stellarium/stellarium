set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

add_library(TelescopeControl_Rts2 STATIC
    TelescopeClientJsonRts2.hpp
    TelescopeClientJsonRts2.cpp
    )

target_link_libraries(TelescopeControl_Rts2
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Gui
    TelescopeControl_common
    )

SET_TARGET_PROPERTIES(TelescopeControl_Rts2 PROPERTIES FOLDER "plugins/TelescopeControl")
