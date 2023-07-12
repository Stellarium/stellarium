set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

FIND_PACKAGE(Qt${QT_VERSION_MAJOR} COMPONENTS Test REQUIRED)

ADD_EXECUTABLE(testSatellites testSatellites.cpp testSatellites.hpp)
TARGET_LINK_LIBRARIES(testSatellites Qt${QT_VERSION_MAJOR}::Test Satellites-static stelMain)
ADD_TEST(testSatellites testSatellites)
SET_TARGET_PROPERTIES(testSatellites PROPERTIES FOLDER "plugins/Satellites/test")

ADD_EXECUTABLE(testOMM testOMM.cpp testOMM.hpp)
TARGET_LINK_LIBRARIES(testOMM Qt${QT_VERSION_MAJOR}::Test Satellites-static stelMain)
ADD_TEST(testOMM testOMM)
SET_TARGET_PROPERTIES(testOMM PROPERTIES FOLDER "plugins/Satellites/test")
FILE(COPY "${CMAKE_SOURCE_DIR}/plugins/Satellites/src/test/test_data.xml" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
FILE(COPY "${CMAKE_SOURCE_DIR}/plugins/Satellites/src/test/test_data.json" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")

ADD_EXECUTABLE(testOMMDateTime testOMMDateTime.cpp testOMMDateTime.hpp)
TARGET_LINK_LIBRARIES(testOMMDateTime Qt${QT_VERSION_MAJOR}::Test Satellites-static stelMain)
ADD_TEST(testOMMDateTime testOMMDateTime)
SET_TARGET_PROPERTIES(testOMMDateTime PROPERTIES FOLDER "plugins/Satellites/test")

ADD_EXECUTABLE(testSGP4 testSGP4.cpp testSGP4.hpp)
TARGET_LINK_LIBRARIES(testSGP4 Qt${QT_VERSION_MAJOR}::Test Satellites-static stelMain)
ADD_TEST(testSGP4 testSGP4)
SET_TARGET_PROPERTIES(testSGP4 PROPERTIES FOLDER "plugins/Satellites/test")
