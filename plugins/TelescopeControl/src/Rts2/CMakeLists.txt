set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

add_library(TelescopeControl_Rts2 STATIC
    TelescopeClientJsonRts2.hpp
    TelescopeClientJsonRts2.cpp
    )

target_link_libraries(TelescopeControl_Rts2
    Qt5::Core
    Qt5::Network
    Qt5::Gui
    TelescopeControl_common
    )

SET_TARGET_PROPERTIES(TelescopeControl_Rts2 PROPERTIES FOLDER "plugins/TelescopeControl")
