set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

find_package(ZLIB REQUIRED)
find_package(Qt5Test)

include_directories($<TARGET_PROPERTY:TelescopeControl-static,INCLUDE_DIRECTORIES>)

add_executable(test_TelescopeControl TestTelescopeControl.hpp TestTelescopeControl.cpp)
target_link_libraries(test_TelescopeControl Qt5::Test TelescopeControl-static stelMain)
