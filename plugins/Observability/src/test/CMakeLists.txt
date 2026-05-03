set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

FIND_PACKAGE(Qt${QT_VERSION_MAJOR} COMPONENTS Test REQUIRED)

ADD_EXECUTABLE(testObservability testObservability.hpp testObservability.cpp)
TARGET_LINK_LIBRARIES(testObservability Qt${QT_VERSION_MAJOR}::Test Observability-static stelMain)
ADD_TEST(testObservability testObservability)
SET_TARGET_PROPERTIES(testObservability PROPERTIES FOLDER "plugins/Observability/test")

