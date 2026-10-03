# Headers intentionally included in the experimental Windows plug-in SDK.
# Paths are relative to the Stellarium src directory.
SET(STELLARIUM_PLUGIN_SDK_API_HEADERS
     "core/StelModule.hpp"
     "core/StelPluginInterface.hpp")

SET(STELLARIUM_PLUGIN_SDK_DEPENDENCY_HEADERS
     "StelMainExport.hpp")

SET(STELLARIUM_PLUGIN_SDK_HEADERS
     ${STELLARIUM_PLUGIN_SDK_API_HEADERS}
     ${STELLARIUM_PLUGIN_SDK_DEPENDENCY_HEADERS})
LIST(SORT STELLARIUM_PLUGIN_SDK_HEADERS)

SET(STELLARIUM_PLUGIN_SDK_QT_COMPONENTS
     Core
     Gui)
