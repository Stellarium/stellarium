set(INDI_MATH_PLUGINS_DIRECTORY "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/indi/MathPlugins")
set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS INDI_MATH_PLUGINS_DIRECTORY="${INDI_MATH_PLUGINS_DIRECTORY}")

option(ALIGNMENT_CONVEX_HULL_DEBUGGING "Alignment subsystem - additional debugging output" OFF)

if(ALIGNMENT_CONVEX_HULL_DEBUGGING)
   set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS CONVEX_HULL_DEBUGGING)
endif(ALIGNMENT_CONVEX_HULL_DEBUGGING)

option(SKYWATCHER_API_USE_INITIAL_JULIAN_DATE "Skywatcher API - use initial Julian date only" OFF)

if(SKYWATCHER_API_USE_INITIAL_JULIAN_DATE)
   set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS USE_INITIAL_JULIAN_DATE)
endif(SKYWATCHER_API_USE_INITIAL_JULIAN_DATE)

##################################################
####### INDI AlignmentDriver shared library ######
##################################################
SET(AlignmentDriver_SRC
    ${CMAKE_SOURCE_DIR}/libs/indibase/alignment/AlignmentSubsystemForDrivers.cpp ;
    ${CMAKE_SOURCE_DIR}/libs/indibase/alignment/AlignmentSubsystemForMathPlugins.h ;
    ${CMAKE_SOURCE_DIR}/libs/indibase/alignment/BasicMathPlugin.cpp ;
    ${CMAKE_SOURCE_DIR}/libs/indibase/alignment/BuiltInMathPlugin.cpp ;
    ${CMAKE_SOURCE_DIR}/libs/indibase/alignment/ConvexHull.cpp ;
    ${CMAKE_SOURCE_DIR}/libs/indibase/alignment/DriverCommon.cpp ;
    ${CMAKE_SOURCE_DIR}/libs/indibase/alignment/InMemoryDatabase.cpp ;
    ${CMAKE_SOURCE_DIR}/libs/indibase/alignment/MapPropertiesToInMemoryDatabase.cpp ;
    ${CMAKE_SOURCE_DIR}/libs/indibase/alignment/MathPlugin.cpp ;
    ${CMAKE_SOURCE_DIR}/libs/indibase/alignment/MathPluginManagement.cpp ;
    ${CMAKE_SOURCE_DIR}/libs/indibase/alignment/TelescopeDirectionVectorSupportFunctions.cpp ;
    ${CMAKE_SOURCE_DIR}/libs/indibase/alignment/Common.cpp)

IF (UNITY_BUILD)
    ENABLE_UNITY_BUILD(AlignmentDriver AlignmentDriver_SRC 15 cpp)
ENDIF ()

IF (CYGWIN)
    SET(AlignmentDriver_SRC ${AlignmentDriver_SRC}
        ${CMAKE_SOURCE_DIR}/libs/lilxml.c
        ${CMAKE_SOURCE_DIR}/libs/indicom.c)

    add_library(AlignmentDriver STATIC ${AlignmentDriver_SRC})
    target_link_libraries(AlignmentDriver dl ${GSL_LIBRARIES} ${NOVA_LIBRARIES})
ELSE ()
    add_library(AlignmentDriver SHARED ${AlignmentDriver_SRC})
    set_target_properties(AlignmentDriver PROPERTIES COMPILE_FLAGS "-fPIC")
    IF (APPLE)
        target_link_libraries(AlignmentDriver dl -L/usr/local/lib ${GSL_LIBRARIES})
    ELSE ()
        # Force linking all referenced libraries because of libgsl is not linked against cblas library on Linux
        SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-as-needed")
        target_link_libraries(AlignmentDriver dl ${GSL_LIBRARIES})
    ENDIF ()
ENDIF ()

if (INDI_BUILD_QT5)
qt5_use_modules(AlignmentDriver Network)
endif(INDI_BUILD_QT5)

set_target_properties(AlignmentDriver PROPERTIES VERSION ${CMAKE_INDI_VERSION_STRING} SOVERSION ${INDI_SOVERSION} OUTPUT_NAME indiAlignmentDriver)
install(TARGETS AlignmentDriver
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES AlignmentSubsystemForMathPlugins.h AlignmentSubsystemForDrivers.h BasicMathPlugin.h BuiltInMathPlugin.h
              ClientAPIForAlignmentDatabase.h ClientAPIForMathPluginManagement.h Common.h ConvexHull.h DriverCommon.h InMemoryDatabase.h MathPlugin.h
              MathPluginManagement.h SVDMathPlugin.h TelescopeDirectionVectorSupportFunctions.h MapPropertiesToInMemoryDatabase.h
        DESTINATION ${INCLUDE_INSTALL_DIR}/libindi/alignment COMPONENT Devel)

##################################################
####### INDI AlignmentClient static library ######
##################################################
set (AlignmentClient_SRCS
        ${CMAKE_SOURCE_DIR}/libs/indibase/alignment/AlignmentSubsystemForClients.cpp
        ${CMAKE_SOURCE_DIR}/libs/indibase/alignment/ClientAPIForAlignmentDatabase.cpp
        ${CMAKE_SOURCE_DIR}/libs/indibase/alignment/ClientAPIForMathPluginManagement.cpp
        ${CMAKE_SOURCE_DIR}/libs/indibase/alignment/Common.h
    )

add_library(AlignmentClient STATIC ${AlignmentClient_SRCS})

if (INDI_BUILD_QT5)
qt5_use_modules(AlignmentClient Network)
endif(INDI_BUILD_QT5)
if (NOT CYGWIN AND NOT WIN32)
SET_TARGET_PROPERTIES(AlignmentClient PROPERTIES COMPILE_FLAGS "-fPIC")
endif(NOT CYGWIN AND NOT WIN32)
set_target_properties(AlignmentClient PROPERTIES OUTPUT_NAME indiAlignmentClient)
install(TARGETS AlignmentClient ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

##################################################
############ LoaderCLient test program ###########
##################################################
#set(LoaderClient_SRCS
#	${CMAKE_SOURCE_DIR}/libs/indibase/alignment/LoaderClient.cpp
#	${CMAKE_SOURCE_DIR}/libs/indibase/alignment/LoaderMain.cpp
#	)

#add_executable(LoaderClient ${LoaderClient_SRCS})

#target_link_libraries(LoaderClient indiclient AlignmentClient)

#install(TARGETS LoaderClient RUNTIME DESTINATION bin)

##################################################
######### MathPluginManager test program #########
##################################################
#set(MathPluginManagerClient_SRCS
#	${CMAKE_SOURCE_DIR}/libs/indibase/alignment/MathPluginManagerClient.cpp
#	${CMAKE_SOURCE_DIR}/libs/indibase/alignment/MathPluginManagerMain.cpp
#	)

#add_executable(MathPluginManagerClient ${MathPluginManagerClient_SRCS})

#target_link_libraries(MathPluginManagerClient indiclient AlignmentClient)

#install(TARGETS MathPluginManagerClient RUNTIME DESTINATION bin)

##################################################
########### Dummy math plugin example ############
##################################################
#set(DummyMathPlugin_SRCS
#	${CMAKE_SOURCE_DIR}/libs/indibase/alignment/DummyMathPlugin.cpp
#	)

#add_library(indi_Dummy_MathPlugin SHARED ${DummyMathPlugin_SRCS})

#target_link_libraries(indi_Dummy_MathPlugin indidriver)

#install(TARGETS indi_Dummy_MathPlugin LIBRARY DESTINATION ${INDI_MATH_PLUGINS_DIRECTORY})

##################################################
################ SVD math plugin #################
##################################################
set(SVDMathPlugin_SRCS
        ${CMAKE_SOURCE_DIR}/libs/indibase/alignment/SVDMathPlugin.cpp
	)

if (CYGWIN)
add_library(indi_SVD_MathPlugin STATIC ${SVDMathPlugin_SRCS})
target_link_libraries(indi_SVD_MathPlugin ${GSL_LIBRARIES} ${NOVA_LIBRARIES})
else(CYGWIN)
add_library(indi_SVD_MathPlugin SHARED ${SVDMathPlugin_SRCS})
set_property(TARGET indi_SVD_MathPlugin APPEND PROPERTY COMPILE_DEFINITIONS SVD_TRANSFORM_MATRIX)
target_link_libraries(indi_SVD_MathPlugin indidriver)
endif(CYGWIN)

install(TARGETS indi_SVD_MathPlugin
    ARCHIVE DESTINATION ${INDI_MATH_PLUGINS_DIRECTORY}
    LIBRARY DESTINATION ${INDI_MATH_PLUGINS_DIRECTORY}
    RUNTIME DESTINATION ${INDI_MATH_PLUGINS_DIRECTORY})

##################################################
############ Skywatcher Alt-Az Mount ################
##################################################
set(skywatcherAltAzMount_SRCS
   ${CMAKE_SOURCE_DIR}/drivers/telescope/skywatcherAPIMount.cpp
   ${CMAKE_SOURCE_DIR}/drivers/telescope/skywatcherAPI.cpp )

add_executable(indi_skywatcherAltAzMount ${skywatcherAltAzMount_SRCS})
target_link_libraries(indi_skywatcherAltAzMount indidriver AlignmentDriver)

install(TARGETS indi_skywatcherAltAzMount RUNTIME DESTINATION bin)


