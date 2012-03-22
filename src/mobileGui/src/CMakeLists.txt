IF(BUILD_DYNAMIC_PLUGINS)
	message(FATAL_ERROR "Cannot currently build MobileGui as a dynamic plugin.")
endif()

SET(mobilegui_SRCS
	MobileGui.hpp
	MobileGui.cpp
	systemdisplayinfo.hpp
	systemdisplayinfo.cpp
	stelwrapper.hpp
	stelwrapper.cpp
	updatesignallingitem.hpp
	updatesignallingitem.cpp)
SET(mobilegui_MOC_HDRS
	MobileGui.hpp
	systemdisplayinfo.hpp
	stelwrapper.hpp)

SET(mobilegui_RES qml/qmlResources.qrc)
QT4_ADD_RESOURCES(mobilegui_RES_CXX ${mobilegui_RES})
	
QT4_WRAP_CPP(mobilegui_MOC_SRCS ${mobilegui_MOC_HDRS})

ADD_LIBRARY(ExternalGui STATIC ${mobilegui_SRCS} ${mobilegui_MOC_SRCS} ${mobilegui_RES_CXX})
TARGET_LINK_LIBRARIES(ExternalGui ${extLinkerOption})
SET_TARGET_PROPERTIES(ExternalGui PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
ADD_DEPENDENCIES(AllStaticPlugins ExternalGui)
