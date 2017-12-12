########### Tutorial three ##############
if (CFITSIO_FOUND)
add_executable(tutorial_three simpleccd.cpp)

target_link_libraries(tutorial_three indidriver)
endif (CFITSIO_FOUND)

