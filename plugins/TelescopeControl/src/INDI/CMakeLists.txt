set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

find_package(Threads REQUIRED)

add_library(TelescopeControl_INDI
    INDIConnection.hpp
    INDIConnection.cpp
    TelescopeClientINDI.hpp
    TelescopeClientINDI.cpp
    )

target_link_libraries(TelescopeControl_INDI
    Qt5::Core
    Qt5::Network
    indiclient
    TelescopeControl_common
    ${CMAKE_THREAD_LIBS_INIT}
    )

target_include_directories(TelescopeControl_INDI INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

#add_subdirectory(test)
