set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

add_library(TelescopeControl_gui
    slewDialog.ui
    telescopeDialog.ui
    telescopeConfigurationDialog.ui
    storedPointsDialog.ui
    SlewDialog.hpp
    SlewDialog.cpp
    TelescopeDialog.hpp
    TelescopeDialog.cpp
    TelescopeConfigurationDialog.hpp
    TelescopeConfigurationDialog.cpp
    StoredPointsDialog.hpp
    StoredPointsDialog.cpp
    )

target_link_libraries(TelescopeControl_gui
    Qt5::Core
    Qt5::Network
    Qt5::Gui
    TelescopeControl_common
    TelescopeControl_INDI
    )

SET_TARGET_PROPERTIES(TelescopeControl_gui PROPERTIES FOLDER "plugins/TelescopeControl")
