set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

add_library(TelescopeControl_common
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
    )

target_link_libraries(TelescopeControl_common
    Qt5::Core
    Qt5::Network
    Qt5::Gui
    Qt5::Widgets
    )
