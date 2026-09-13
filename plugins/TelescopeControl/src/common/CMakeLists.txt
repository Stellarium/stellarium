set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

IF(WIN32)
    SET(TelescopeControl_ASCOM_common_SRC
        ASCOMSupport.hpp
        ASCOMSupport.cpp
        OLE.hpp
        OLE.cpp
    )
ENDIF(WIN32)

ADD_LIBRARY(TelescopeControl_common STATIC
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
    TcpConnection.hpp
    TcpConnection.cpp
    InterpolatedPosition.hpp
    InterpolatedPosition.cpp
    ${TelescopeControl_ASCOM_common_SRC}
    )

TARGET_LINK_LIBRARIES(TelescopeControl_common
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    )

IF(${QT_VERSION_MAJOR} EQUAL 5 AND ${CMAKE_SYSTEM_NAME} STREQUAL "SunOS")
    # Fix linking on SunOS platform (OpenIndiana/Illumos/Solaris/etc.) for Qt5-based package
    TARGET_LINK_LIBRARIES(TelescopeControl_common socket)
ENDIF()

SET_TARGET_PROPERTIES(TelescopeControl_common PROPERTIES FOLDER "plugins/TelescopeControl")
