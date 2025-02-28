set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
find_package(GettextPo REQUIRED)

add_executable(skyculture-converter
                Utils.cpp
                NamesOldLoader.cpp
                AsterismOldLoader.cpp
                DescriptionOldLoader.cpp
                ConstellationOldLoader.cpp
                main.cpp
              )
target_link_libraries(skyculture-converter Qt::Core Qt::Gui GettextPo::GettextPo)

IF(APPLE)
     INSTALL(TARGETS skyculture-converter DESTINATION MacOS)
     INSTALL(CODE
	  "
	  EXECUTE_PROCESS(
	       COMMAND \"${MACDEPLOYQT_COMMAND}\"
		    \"\${CMAKE_CURRENT_BINARY_DIR}/skyculture-converter.app\"
	       WORKING_DIRECTORY \"\${CMAKE_CURRENT_BINARY_DIR}\"
	       OUTPUT_FILE \"\${CMAKE_CURRENT_BINARY_DIR}/macdeployqt.log\"
	       ERROR_FILE \"\${CMAKE_CURRENT_BINARY_DIR}/macdeployqt.err\"
	  )
	  MESSAGE(STATUS \"Running macdeployqt...\")
	  "
     )
else()
     install(TARGETS skyculture-converter DESTINATION bin)
endif()
