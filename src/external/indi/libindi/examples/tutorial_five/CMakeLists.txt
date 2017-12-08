########### Tutorial five - dome driver ##############
add_executable(tutorial_dome dome.cpp)

target_link_libraries(tutorial_dome indidriver)

########### Tutorial five - rain driver ##############
add_executable(tutorial_rain raindetector.cpp)

target_link_libraries(tutorial_rain indidriver)


