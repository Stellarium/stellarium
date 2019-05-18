set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

add_library(TelescopeControl_NexStar STATIC
    NexStarCommand.hpp
    NexStarCommand.cpp
    NexStarConnection.hpp
    NexStarConnection.cpp
    TelescopeClientDirectNexStar.hpp
    TelescopeClientDirectNexStar.cpp
    )

target_link_libraries(TelescopeControl_NexStar
    Qt5::Core
    Qt5::Network
    Qt5::Gui
    TelescopeControl_common
    )

SET_TARGET_PROPERTIES(TelescopeControl_NexStar PROPERTIES FOLDER "plugins/TelescopeControl")
