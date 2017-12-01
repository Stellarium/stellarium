FIND_PACKAGE(INDI REQUIRED client)
FIND_PACKAGE(Threads REQUIRED)
FIND_PACKAGE(ZLIB REQUIRED)

add_library(TelescopeControl_INDI
    INDIConnection.hpp
    INDIConnection.cpp
    TelescopeClientINDI.hpp
    TelescopeClientINDI.cpp
    )

target_link_libraries(TelescopeControl_INDI
    ${CMAKE_THREAD_LIBS_INIT}
    ${ZLIB_LIBRARIES}
    ${INDI_LIBRARIES}
    )
