cmake_minimum_required(VERSION 3.10.0)
project(skyculture-converter VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(${CMAKE_CXX_COMPILER_ID} MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror=return-type -Wall -Wextra")
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
find_package(Qt5 5.15 REQUIRED Core OpenGL)

add_executable(skyculture-converter
                NamesOldLoader.cpp
                AsterismOldLoader.cpp
                DescriptionOldLoader.cpp
                ConstellationOldLoader.cpp
                main.cpp
              )
target_link_libraries(skyculture-converter Qt5::Core Qt5::Gui gettextpo)
