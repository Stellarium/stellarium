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
   
   add_custom_target ( build_apk_debug
      COMMAND ant debug
   )
   
   add_custom_target ( build_apk_release
      COMMAND ant release
   )
   
   add_custom_target ( build_apk_debug_no_assets
      COMMAND ant debug
   )

   add_dependencies(build_apk_debug android_resources)
   add_dependencies(build_apk_release android_resources)
   add_dependencies(build_apk_debug_no_assets android_resources_no_assets)
   
endif(BUILD_FOR_ANDROID)
