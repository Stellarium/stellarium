set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

add_library(TelescopeControl_Fs2 STATIC
    Fs2Command.hpp
    Fs2Command.cpp
    Fs2Connection.hpp
    Fs2Connection.cpp
    TelescopeClientDirectFs2.hpp
    TelescopeClientDirectFs2.cpp
    )

target_link_libraries(TelescopeControl_Fs2
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::OpenGL
    TelescopeControl_common
    )

SET_TARGET_PROPERTIES(TelescopeControl_Fs2 PROPERTIES FOLDER "plugins/TelescopeControl")
