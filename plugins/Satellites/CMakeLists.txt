SET(SATELLITES_VERSION "0.6.4")

SET(CMAKE_INSTALL_PREFIX $ENV{HOME}/.stellarium)


########### Generate plugin_config.h ###########
SET(PLUGIN_VERSION ${SATELLITES_VERSION})
CONFIGURE_FILE(plugin_config.h.cmake ${CMAKE_BINARY_DIR}/plugins/Satellites/src/plugin_config.h)

ADD_SUBDIRECTORY( src )
INSTALL(FILES DESTINATION "modules/Satellites")
