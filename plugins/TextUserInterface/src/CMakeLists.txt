INCLUDE_DIRECTORIES(.)

LINK_DIRECTORIES(${BUILD_DIR}/src)

SET(TextUserInterface_SRCS
     DummyDialog.hpp
     DummyDialog.cpp
     TuiNode.hpp
     TuiNode.cpp
     TuiNodeActivate.hpp
     TuiNodeActivate.cpp
     TuiNodeEditable.hpp
     TuiNodeEditable.cpp
     TuiNodeBool.hpp
     TuiNodeBool.cpp
     TuiNodeInt.hpp
     TuiNodeInt.cpp
     TuiNodeDouble.hpp
     TuiNodeDouble.cpp
     TuiNodeFloat.hpp
     TuiNodeFloat.cpp
     TuiNodeDateTime.hpp
     TuiNodeDateTime.cpp
     TuiNodeColor.hpp
     TuiNodeColor.cpp
     TuiNodeEnum.hpp
     TuiNodeEnum.cpp
     TextUserInterface.hpp
     TextUserInterface.cpp
)

ADD_LIBRARY(TextUserInterface-static STATIC ${TextUserInterface_SRCS})
TARGET_LINK_LIBRARIES(TextUserInterface-static Qt5::Core Qt5::Widgets)
# The library target "TextUserInterface-static" has a default OUTPUT_NAME of "TextUserInterface-static", so change it.
SET_TARGET_PROPERTIES(TextUserInterface-static PROPERTIES OUTPUT_NAME "TextUserInterface")
SET_TARGET_PROPERTIES(TextUserInterface-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
ADD_DEPENDENCIES(AllStaticPlugins TextUserInterface-static)

SET_TARGET_PROPERTIES(TextUserInterface-static PROPERTIES FOLDER "plugins/TextUserInterface")
