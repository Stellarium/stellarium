set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

FIND_PACKAGE(Qt5Test)

ADD_EXECUTABLE(testHistoricalSupernovae testHistoricalSupernovae.cpp testHistoricalSupernovae.hpp)
TARGET_LINK_LIBRARIES(testHistoricalSupernovae Qt5::Test Supernovae-static stelMain)
ADD_TEST(testHistoricalSupernovae testHistoricalSupernovae)
SET_TARGET_PROPERTIES(testHistoricalSupernovae PROPERTIES FOLDER "plugins/Supernovae/test")

