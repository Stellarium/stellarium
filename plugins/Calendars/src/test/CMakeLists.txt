set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

FIND_PACKAGE(Qt5Test)

ADD_EXECUTABLE(testCalendars testCalendars.hpp testCalendars.cpp)
TARGET_LINK_LIBRARIES(testCalendars Qt5::Test Calendars-static stelMain)
    #ADD_DEPENDENCIES(buildTests testCalendars)
ADD_TEST(testCalendars testCalendars)
SET_TARGET_PROPERTIES(testCalendars PROPERTIES FOLDER "plugins/Calendars/src/test")