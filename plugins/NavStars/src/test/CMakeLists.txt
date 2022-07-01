set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

FIND_PACKAGE(Qt${QT_VERSION_MAJOR} COMPONENTS Test REQUIRED)

ADD_EXECUTABLE(testNavStars testNavStars.cpp testNavStars.hpp)
TARGET_LINK_LIBRARIES(testNavStars Qt${QT_VERSION_MAJOR}::Test NavStars-static stelMain)
ADD_TEST(testNavStars testNavStars)
SET_TARGET_PROPERTIES(testNavStars PROPERTIES FOLDER "plugins/NavStars/test")

