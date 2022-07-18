set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

FIND_PACKAGE(Qt${QT_VERSION_MAJOR} COMPONENTS Test REQUIRED)

ADD_EXECUTABLE(testCalendars testCalendars.hpp testCalendars.cpp)
TARGET_LINK_LIBRARIES(testCalendars Qt${QT_VERSION_MAJOR}::Test Calendars-static stelMain)
    #ADD_DEPENDENCIES(buildTests testCalendars)
ADD_TEST(testCalendars testCalendars)
SET_TARGET_PROPERTIES(testCalendars PROPERTIES FOLDER "plugins/Calendars/src/test")
