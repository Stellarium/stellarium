set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

FIND_PACKAGE(Qt${QT_VERSION_MAJOR} COMPONENTS Test REQUIRED)

add_executable(testTelescopeControl_ASCOM testTelescopeControl_ASCOM.cpp testTelescopeControl_ASCOM.hpp)
target_link_libraries(testTelescopeControl_ASCOM Qt${QT_VERSION_MAJOR}::Test TelescopeControl_ASCOM stelMain)
add_test(testTelescopeControl_ASCOM testTelescopeControl_ASCOM)
SET_TARGET_PROPERTIES(testTelescopeControl_ASCOM PROPERTIES FOLDER "plugins/TelescopeControl/test")

