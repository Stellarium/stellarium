set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

add_library(TelescopeControl_Lx200
    Lx200Command.hpp
    Lx200Command.cpp
    Lx200Connection.hpp
    Lx200Connection.cpp
    TelescopeClientDirectLx200.hpp
    TelescopeClientDirectLx200.cpp
    )

target_link_libraries(TelescopeControl_Lx200
    Qt5::Core
    Qt5::Network
    Qt5::Gui
    TelescopeControl_common
    )
