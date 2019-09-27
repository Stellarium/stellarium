set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

FIND_PACKAGE(Qt5Test)

ADD_EXECUTABLE(testNavStars testNavStars.cpp testNavStars.hpp)
TARGET_LINK_LIBRARIES(testNavStars Qt5::Test NavStars-static stelMain)
ADD_TEST(testNavStars testNavStars)
SET_TARGET_PROPERTIES(testNavStars PROPERTIES FOLDER "plugins/NavStars/test")

