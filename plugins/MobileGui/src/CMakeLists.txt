SET(stellarium_nogui_SRCS
	StelNoGui.hpp
	StelNoGui.cpp)
SET(stellarium_nogui_MOC_HDRS
	StelNoGui.hpp)
	
QT4_WRAP_CPP(stellarium_nogui_MOC_SRCS ${stellarium_nogui_MOC_HDRS})

ADD_LIBRARY(externalGui STATIC ${stellarium_nogui_SRCS} ${stellarium_nogui_MOC_SRCS})
TARGET_LINK_LIBRARIES(externalGui ${extLinkerOption})
SET_TARGET_PROPERTIES(externalGui PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
ADD_DEPENDENCIES(AllStaticPlugins externalGui)