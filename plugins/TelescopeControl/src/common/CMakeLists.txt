set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

add_library(TelescopeControl_common
    TelescopeControlGlobals.hpp
    TelescopeControl.hpp
    TelescopeControl.cpp
    LogFile.hpp
    LogFile.cpp
    Socket.hpp
    Socket.cpp
    Server.hpp
    Server.cpp
    Connection.hpp
    Connection.cpp
    SerialPort.hpp
    SerialPort.cpp
    InterpolatedPosition.hpp
    InterpolatedPosition.cpp
    TelescopeClient.hpp
    TelescopeClient.cpp
    )

target_link_libraries(TelescopeControl_common
    Qt5::Core
    Qt5::Network
    Qt5::Gui
    Qt5::Widgets
    )
