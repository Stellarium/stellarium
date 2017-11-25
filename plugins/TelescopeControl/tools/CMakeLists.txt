FIND_PACKAGE(INDI REQUIRED client)

include_directories($<TARGET_PROPERTY:TelescopeControl-static,INCLUDE_DIRECTORIES>)

ADD_EXECUTABLE(INDITester INDITesterApp.cpp)
TARGET_LINK_LIBRARIES(INDITester ${INDI_LIBRARIES} TelescopeControl-static)
