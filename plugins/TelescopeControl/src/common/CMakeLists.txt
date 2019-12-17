set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

IF(WIN32)
    SET(TelescopeControl_ASCOM_common_SRC
        ASCOMSupport.hpp
        ASCOMSupport.cpp
        OLE.hpp
        OLE.cpp
    )
ENDIF()

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
    ${TelescopeControl_ASCOM_common_SRC}
    )

target_link_libraries(TelescopeControl_common
    Qt5::Core
    Qt5::Network
    Qt5::Gui
    Qt5::Widgets
    )

SET_TARGET_PROPERTIES(TelescopeControl_common PROPERTIES FOLDER "plugins/TelescopeControl")
