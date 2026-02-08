set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

FIND_PACKAGE(Qt${QT_VERSION_MAJOR} COMPONENTS Test REQUIRED)

add_executable(testTelescopeControl_Alpaca testTelescopeControl_Alpaca.cpp testTelescopeControl_Alpaca.hpp)
target_link_libraries(testTelescopeControl_Alpaca Qt${QT_VERSION_MAJOR}::Test TelescopeControl_Alpaca stelMain)
add_test(testTelescopeControl_Alpaca testTelescopeControl_Alpaca)
SET_TARGET_PROPERTIES(testTelescopeControl_Alpaca PROPERTIES FOLDER "plugins/TelescopeControl/test")

