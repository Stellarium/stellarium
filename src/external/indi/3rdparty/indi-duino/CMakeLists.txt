cmake_minimum_required(VERSION 2.4.7)
PROJECT(indi_duino CXX C)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/")
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake_modules/")
set(FIRMATA_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libfirmata/src/")
include(GNUInstallDirs)

set (DUINO_VERSION_MAJOR 0)
set (DUINO_VERSION_MINOR 2)
 
find_package(INDI REQUIRED)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h )
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/indi_duino.xml.cmake ${CMAKE_CURRENT_BINARY_DIR}/indi_duino.xml )

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${INDI_INCLUDE_DIR})
include_directories(${FIRMATA_INCLUDE_DIR})

include(CMakeCommon)

################# libfirmata ############################
set (firmata_VERSION_MAJOR 1)
set (firmata_VERSION_MINOR 0)
set (firmata_SRCS
        ${CMAKE_CURRENT_SOURCE_DIR}/libfirmata/src/firmata.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/libfirmata/src/arduino.cpp
)
SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_SOURCE_DIR}/libfirmata/src/firmata.cpp PROPERTIES COMPILE_FLAGS "-Wno-error")
SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_SOURCE_DIR}/libfirmata/src/arduino.cpp PROPERTIES COMPILE_FLAGS "-Wno-error")
add_library(firmata ${firmata_SRCS})
SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_SOURCE_DIR}/libfirmata/examples/blink_pin.cpp PROPERTIES COMPILE_FLAGS "-Wno-error")
add_executable(blink_pin ${CMAKE_CURRENT_SOURCE_DIR}/libfirmata/examples/blink_pin.cpp)
target_link_libraries (blink_pin firmata)
add_executable(read_digital_pin ${CMAKE_CURRENT_SOURCE_DIR}/libfirmata/examples/read_digital_pin.cpp)
target_link_libraries (read_digital_pin firmata)
add_executable(read_analog_pin ${CMAKE_CURRENT_SOURCE_DIR}/libfirmata/examples/read_analog_pin.cpp)
target_link_libraries (read_analog_pin firmata)
add_executable(ask_digital_pin ${CMAKE_CURRENT_SOURCE_DIR}/libfirmata/examples/ask_digital_pin.cpp)
target_link_libraries (ask_digital_pin firmata)
add_executable(ask_analog_pin ${CMAKE_CURRENT_SOURCE_DIR}/libfirmata/examples/ask_analog_pin.cpp)
target_link_libraries (ask_analog_pin firmata)
add_executable(read_string ${CMAKE_CURRENT_SOURCE_DIR}/libfirmata/examples/read_string.cpp)
target_link_libraries (read_string firmata)

##################### indi arduino #####################
set(indiduino_SRCS
   ${CMAKE_CURRENT_SOURCE_DIR}/indiduino.cpp
   )

add_executable(indi_duino ${indiduino_SRCS})
target_link_libraries(indi_duino ${INDI_LIBRARIES} firmata)

install(TARGETS indi_duino RUNTIME DESTINATION bin)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/indi_duino.xml DESTINATION ${INDI_DATA_DIR})
################### DEVICES XML  #####################
add_subdirectory(devices)

