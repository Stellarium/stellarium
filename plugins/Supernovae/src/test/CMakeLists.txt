set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

FIND_PACKAGE(Qt${QT_VERSION_MAJOR} COMPONENTS Test REQUIRED)

ADD_EXECUTABLE(testHistoricalSupernovae testHistoricalSupernovae.cpp testHistoricalSupernovae.hpp)
TARGET_LINK_LIBRARIES(testHistoricalSupernovae Qt${QT_VERSION_MAJOR}::Test Supernovae-static stelMain)
ADD_TEST(testHistoricalSupernovae testHistoricalSupernovae)
SET_TARGET_PROPERTIES(testHistoricalSupernovae PROPERTIES FOLDER "plugins/Supernovae/test")

