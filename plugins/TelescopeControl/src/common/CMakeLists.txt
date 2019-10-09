set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

add_library(TelescopeControl_common STATIC
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

IF(WIN32)
add_library(TelescopeControl_ASCOM_common STATIC
    ASCOMSupport.hpp
    ASCOMSupport.cpp
    OLE.hpp
    OLE.cpp
    )

target_link_libraries(TelescopeControl_common
    TelescopeControl_ASCOM_common
    )
ENDIF(WIN32)

SET_TARGET_PROPERTIES(TelescopeControl_common PROPERTIES FOLDER "plugins/TelescopeControl")
