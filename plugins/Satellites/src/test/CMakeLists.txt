set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

FIND_PACKAGE(Qt${QT_VERSION_MAJOR} COMPONENTS Test REQUIRED)

ADD_EXECUTABLE(testSatellites testSatellites.cpp testSatellites.hpp)
TARGET_LINK_LIBRARIES(testSatellites Qt${QT_VERSION_MAJOR}::Test Satellites-static stelMain)
ADD_TEST(testSatellites testSatellites)
SET_TARGET_PROPERTIES(testSatellites PROPERTIES FOLDER "plugins/Satellites/test")

