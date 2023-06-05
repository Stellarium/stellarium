set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

FIND_PACKAGE(Qt${QT_VERSION_MAJOR} COMPONENTS Test REQUIRED)

ADD_EXECUTABLE(testSolarSystemEditor testSolarSystemEditor.cpp testSolarSystemEditor.hpp)
TARGET_LINK_LIBRARIES(testSolarSystemEditor Qt${QT_VERSION_MAJOR}::Test SolarSystemEditor-static stelMain)
ADD_TEST(testSolarSystemEditor testSolarSystemEditor)
SET_TARGET_PROPERTIES(testSolarSystemEditor PROPERTIES FOLDER "plugins/SolarSystemEditor/test")

