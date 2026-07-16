cmake_minimum_required(VERSION 3.10)
project(MainWindowTest)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(gui)

add_executable(${PROJECT_NAME} mainwindow_test.cpp)

target_link_libraries(${PROJECT_NAME} Qt5::Test)