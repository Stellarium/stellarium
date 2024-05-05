
# to generate manual page:
# pod2man --section=1 -r ${VERSION} --center=" " stellarium.pod stellarium.1

IF(UNIX AND NOT WIN32 AND NOT APPLE)
     ADD_CUSTOM_TARGET(ManPages ALL)

     CONFIGURE_FILE(
          ${PROJECT_SOURCE_DIR}/cmake/stellarium.pod.cmake
          ${CMAKE_CURRENT_BINARY_DIR}/stellarium.pod
     )

     ADD_CUSTOM_COMMAND(
          TARGET ManPages
          COMMAND pod2man
          ARGS -s 1 -r ${VERSION} -center=" " ${CMAKE_CURRENT_BINARY_DIR}/stellarium.pod ${CMAKE_CURRENT_BINARY_DIR}/stellarium.1
          OUTPUTS ${CMAKE_CURRENT_BINARY_DIR}/stellarium.1
     )

     ADD_CUSTOM_COMMAND(
          TARGET ManPages
          DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/stellarium.1
     )

     # default directory for manual page
     IF(${CMAKE_SYSTEM_NAME} MATCHES "(DragonFly|NetBSD|OpenBSD)")
          SET(STELLARIUM_MANDIR "man")
     ELSE()
          SET(STELLARIUM_MANDIR "share/man")
     ENDIF()

     IF(CMAKE_INSTALL_MANDIR)
          # use custom directory for manual page
          INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/stellarium.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
     ELSE()
          # use default directory for manual page
          INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/stellarium.1 DESTINATION ${STELLARIUM_MANDIR}/man1)
     ENDIF()
ENDIF()


