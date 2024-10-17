FIND_PACKAGE(exiv2)

CPMFindPackage(NAME NLopt
               URL https://github.com/stevengj/nlopt/archive/refs/tags/v2.7.1.tar.gz
               URL_HASH SHA256=db88232fa5cef0ff6e39943fc63ab6074208831dc0031cf1545f6ecd31ae2a1a
               EXCLUDE_FROM_ALL yes
               OPTIONS "NLOPT_CXX ON" "NLOPT_FORTRAN OFF" "NLOPT_GUILE OFF" "NLOPT_MATLAB OFF" "NLOPT_OCTAVE OFF" "NLOPT_PYTHON OFF" "NLOPT_SWIG OFF" "NLOPT_TESTS OFF"
              )

INCLUDE_DIRECTORIES(
     .
     gui
     ${CMAKE_BINARY_DIR}/plugins/LensDistortionEstimator/src
     ${CMAKE_BINARY_DIR}/plugins/LensDistortionEstimator/src/gui
)

LINK_DIRECTORIES(${CMAKE_BINARY_DIR}/src)

SET(LensDistortionEstimator_SRCS
     LensDistortionEstimator.hpp
     LensDistortionEstimator.cpp
     gui/LensDistortionEstimatorDialog.hpp
     gui/LensDistortionEstimatorDialog.cpp
)

SET(LensDistortionEstimator_UIS
     gui/lensDistortionEstimatorDialog.ui
)

SET(LensDistortionEstimator_RES
     ../resources/LensDistortionEstimator.qrc
)

IF (${QT_VERSION_MAJOR} EQUAL "5")
     QT5_WRAP_UI(LensDistortionEstimator_UIS_H ${LensDistortionEstimator_UIS})
     QT5_ADD_RESOURCES(LensDistortionEstimator_RES_CXX ${LensDistortionEstimator_RES})
ELSE()
     QT_WRAP_UI(LensDistortionEstimator_UIS_H ${LensDistortionEstimator_UIS})
     QT_ADD_RESOURCES(LensDistortionEstimator_RES_CXX ${LensDistortionEstimator_RES})
ENDIF()

ADD_LIBRARY(LensDistortionEstimator-static STATIC ${LensDistortionEstimator_SRCS} ${LensDistortionEstimator_RES_CXX} ${LensDistortionEstimator_UIS_H})
TARGET_LINK_LIBRARIES(LensDistortionEstimator-static Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::OpenGL Qt${QT_VERSION_MAJOR}::Charts)
SET_TARGET_PROPERTIES(LensDistortionEstimator-static PROPERTIES OUTPUT_NAME "LensDistortionEstimator")
SET_TARGET_PROPERTIES(LensDistortionEstimator-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
if(exiv2_FOUND)
     MESSAGE(STATUS "Found exiv2 ${exiv2_VERSION_MAJOR}.${exiv2_VERSION_MINOR}.${exiv2_VERSION_PATCH}")
     TARGET_COMPILE_OPTIONS(LensDistortionEstimator-static PRIVATE "-DHAVE_EXIV2")
     IF(${exiv2_VERSION_MAJOR}.${exiv2_VERSION_MINOR}.${exiv2_VERSION_PATCH} VERSION_GREATER_EQUAL 0.28.1)
	     TARGET_LINK_LIBRARIES(LensDistortionEstimator-static Exiv2::exiv2lib)
     ELSE()
	     TARGET_LINK_LIBRARIES(LensDistortionEstimator-static exiv2lib)
     ENDIF()
endif()

TARGET_COMPILE_OPTIONS(LensDistortionEstimator-static PRIVATE "-DHAVE_NLOPT")
IF(NLopt_ADDED)
	SET_TARGET_PROPERTIES(nlopt PROPERTIES C_EXTENSIONS ON)
	ADD_DEPENDENCIES(LensDistortionEstimator-static generate-cpp)
	TARGET_INCLUDE_DIRECTORIES(LensDistortionEstimator-static
	                           PRIVATE ${CPM_PACKAGE_NLopt_BINARY_DIR} ${CPM_PACKAGE_NLopt_SOURCE_DIR}/src/api)
	TARGET_LINK_LIBRARIES(LensDistortionEstimator-static nlopt)
ELSE()
	# NLOPT_LIBRARIES variable is not set by CPM (unlike find_package; see
	# https://github.com/cpm-cmake/CPM.cmake/issues/132), and the imported target
	# may appear to be NLopt::nlopt or NLopt::nlopt_cxx, and using any one of these
	# is incompatible with versions of nlopt where the other would be found. So
	# let's check which one we should use.
	IF(NLopt_DIR MATCHES "nlopt_cxx$")
		set(NLOPT_LIBRARIES NLopt::nlopt_cxx)
	ELSEIF(NLopt_DIR MATCHES "nlopt$")
		set(NLOPT_LIBRARIES NLopt::nlopt)
	ENDIF()
	TARGET_LINK_LIBRARIES(LensDistortionEstimator-static ${NLOPT_LIBRARIES})
ENDIF()

ADD_DEPENDENCIES(AllStaticPlugins LensDistortionEstimator-static)

SET_TARGET_PROPERTIES(LensDistortionEstimator-static PROPERTIES FOLDER "plugins/LensDistortionEstimator")
