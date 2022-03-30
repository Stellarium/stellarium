set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

SET(TelescopeControl_GUI_UIS
    slewDialog.ui
    telescopeDialog.ui
    telescopeConfigurationDialog.ui
    storedPointsDialog.ui
    )

QT_WRAP_UI(TelescopeControl_GUI_UIS_H ${TelescopeControl_GUI_UIS})

add_library(TelescopeControl_gui STATIC
    SlewDialog.hpp
    SlewDialog.cpp
    TelescopeDialog.hpp
    TelescopeDialog.cpp
    TelescopeConfigurationDialog.hpp
    TelescopeConfigurationDialog.cpp
    StoredPointsDialog.hpp
    StoredPointsDialog.cpp
    ${TelescopeControl_GUI_UIS_H}
    )

target_link_libraries(TelescopeControl_gui
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Gui
    TelescopeControl_common
    TelescopeControl_INDI    
    )

IF(WIN32)
target_link_libraries(TelescopeControl_gui
    TelescopeControl_ASCOM
    )
ENDIF(WIN32)

SET_TARGET_PROPERTIES(TelescopeControl_gui PROPERTIES FOLDER "plugins/TelescopeControl")
