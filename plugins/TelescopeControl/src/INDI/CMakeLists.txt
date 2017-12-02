set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

find_package(INDI REQUIRED client)
find_package(Threads REQUIRED)
find_package(ZLIB REQUIRED)

add_library(TelescopeControl_INDI
    INDIConnection.hpp
    INDIConnection.cpp
    TelescopeClientINDI.hpp
    TelescopeClientINDI.cpp
    )

target_link_libraries(TelescopeControl_INDI
    Qt5::Core
    Qt5::Network
    Qt5::Gui
    ${INDI_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${ZLIB_LIBRARIES}
    TelescopeControl_common
    )
