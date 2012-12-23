if (BUILD_FOR_ANDROID)
   
   FILE(GLOB SRC_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "src/*/*/*/*")
   
   set(ANDROID_PERMISSIONS_EXPANDED "")
   #autoset permissions; allows individual plugins or other build options to set extra permissions. Could be handy.
   #foreach ( PERMISSION ${ANDROID_PERMISSIONS})
   #   set(ANDROID_PERMISSIONS_EXPANDED "${ANDROID_PERMISSIONS_EXPANDED}<uses-permission\tandroid:name=\\\"android.permission.${PERMISSION}\\\"/>" )
   #endforeach()

   SET(ANDROID_VERSION_NAME ${PACKAGE_VERSION})
   SET(ANDROID_PERMISSIONS ${ANDROID_PERMISSIONS_EXPANDED})
   CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/AndroidManifest.xml.cmake ${CMAKE_CURRENT_BINARY_DIR}/AndroidManifest.xml @ONLY)

   add_custom_command (
      OUTPUT build.xml
	  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/AndroidManifest.xml ${SRC_FILES}
      COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
      COMMAND android update project -t android-${ANDROID_API_LEVEL} --name Stellarium -p ${CMAKE_CURRENT_BINARY_DIR}
   )

   add_custom_target( android_resources
      DEPENDS build.xml stellarium
	  COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/assets
      COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/assets
      COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/libs/${ARM_TARGET}
      COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_INSTALL_PREFIX}/bin/libstellarium.so ${CMAKE_CURRENT_BINARY_DIR}/libs/${ARM_TARGET}/
      COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_INSTALL_PREFIX}/share ${CMAKE_CURRENT_BINARY_DIR}/assets
   )
   
   add_custom_target( android_resources_no_assets
      DEPENDS build.xml stellarium
	  COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/assets
      COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/assets
      COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/libs/${ARM_TARGET}
      COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_INSTALL_PREFIX}/bin/libstellarium.so ${CMAKE_CURRENT_BINARY_DIR}/libs/${ARM_TARGET}/
   )
   
   FIND_HOST_PROGRAM(ADB_LOCATION NAMES adb adb.exe)
  
   if (NOT ADB_LOCATION)
     message(WARNING "Could not find adb. Add it to the path if you want to use the deploy/run_apk targets")
   endif()
  
   add_custom_target ( build_apk_debug	  
	  #copy the library to obj, then strip the symbols from it
	  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/obj/local/${ARM_TARGET}/
	  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/libs/${ARM_TARGET}/libstellarium.so ./obj/local/${ARM_TARGET}/
	  COMMAND ${CMAKE_STRIP} ${CMAKE_CURRENT_BINARY_DIR}/libs/${ARM_TARGET}/libstellarium.so
      COMMAND ${ANT_LOCATION} debug
   )
   
   add_custom_target ( build_apk_release
      COMMAND ${ANT_LOCATION} release
   )
   
   add_custom_target ( build_apk_debug_no_assets
	  #copy the library to obj, then strip the symbols from it
	  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/obj/local/${ARM_TARGET}/
	  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/libs/${ARM_TARGET}/libstellarium.so ./obj/local/${ARM_TARGET}/
	  COMMAND ${CMAKE_STRIP} ${CMAKE_CURRENT_BINARY_DIR}/libs/${ARM_TARGET}/libstellarium.so
      COMMAND ${ANT_LOCATION} debug
   )
   
   add_custom_target ( deploy_debug_apk
      COMMAND ${ADB_LOCATION} uninstall org.stellarium.stellarium
      COMMAND ${ADB_LOCATION} install -r ${CMAKE_CURRENT_BINARY_DIR}/bin/Stellarium-debug.apk
	)
	
   add_custom_target ( run_apk
	  COMMAND ${ADB_LOCATION} shell am start -n org.stellarium.stellarium/org.kde.necessitas.origo.QtActivity
	)
	
	IF(${CMAKE_BUILD_TYPE} MATCHES "Debug")	
		# 1. generate a fake Android.mk
		file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/jni/Android.mk "APP_ABI := ${ARM_TARGET}\n")

		# 2. generate gdb.setup
		get_directory_property(INCLUDE_DIRECTORIES DIRECTORY . INCLUDE_DIRECTORIES)
		string(REGEX REPLACE ";" " " INCLUDE_DIRECTORIES "${INCLUDE_DIRECTORIES}")
		file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/libs/${ARM_TARGET}/gdb.setup "set solib-search-path ${CMAKE_CURRENT_BINARY_DIR}/obj/local/${ARM_TARGET}\n")
		file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/libs/${ARM_TARGET}/gdb.setup "directory ${INCLUDE_DIRECTORIES}\n")

		# 3. copy gdbserver executable
                file(COPY ${ANDROID_NDK}/prebuilt/android-arm/gdbserver/gdbserver DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/libs/${ARM_TARGET}/)
	ENDIF()
	
	#also possible:
	#$ADB clear org.stellarium.stellarium
    #$ADB shell am start -n org.stellarium.stellarium/org.kde.necessitas.origo.FirstRunActivity

   add_dependencies(build_apk_debug android_resources)
   add_dependencies(build_apk_release android_resources)
   add_dependencies(build_apk_debug_no_assets android_resources_no_assets)
   
endif(BUILD_FOR_ANDROID)
