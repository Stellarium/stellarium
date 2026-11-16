set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

FIND_PACKAGE(Qt${QT_VERSION_MAJOR} COMPONENTS Test REQUIRED)

ADD_EXECUTABLE(testRemoteControl testRemoteControl.cpp testRemoteControl.hpp)
TARGET_LINK_LIBRARIES(testRemoteControl Qt${QT_VERSION_MAJOR}::Network Qt${QT_VERSION_MAJOR}::Test RemoteControl-static stelMain)
ADD_TEST(testRemoteControl testRemoteControl)
SET_TARGET_PROPERTIES(testRemoteControl PROPERTIES FOLDER "plugins/RemoteControl/test")
