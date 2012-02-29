SET(stellarium_nogui_SRCS
	MobileGui.hpp
	MobileGui.cpp)
SET(stellarium_nogui_MOC_HDRS
	MobileGui.hpp)
	
QT4_WRAP_CPP(stellarium_nogui_MOC_SRCS ${stellarium_nogui_MOC_HDRS})

ADD_LIBRARY(ExternalGui STATIC ${stellarium_nogui_SRCS} ${stellarium_nogui_MOC_SRCS})
TARGET_LINK_LIBRARIES(ExternalGui ${extLinkerOption})
SET_TARGET_PROPERTIES(ExternalGui PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
ADD_DEPENDENCIES(AllStaticPlugins ExternalGui)
