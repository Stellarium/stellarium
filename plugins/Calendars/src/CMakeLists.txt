INCLUDE_DIRECTORIES(. gui)

LINK_DIRECTORIES(${BUILD_DIR}/src)

SET(Calendars_SRCS
    Calendars.hpp
    Calendars.cpp
    gui/CalendarsDialog.hpp
    gui/CalendarsDialog.cpp
    Calendar.hpp
    Calendar.cpp
    JulianCalendar.hpp
    JulianCalendar.cpp
    GregorianCalendar.hpp
    GregorianCalendar.cpp
    ISOCalendar.hpp
    ISOCalendar.cpp
    MayaLongCountCalendar.hpp
    MayaLongCountCalendar.cpp
    MayaHaabCalendar.hpp
    MayaHaabCalendar.cpp
    MayaTzolkinCalendar.hpp
    MayaTzolkinCalendar.cpp
)

SET(Calendars_UIS
     gui/calendarsDialog.ui
)

QT5_WRAP_UI(Calendars_UIS_H ${Calendars_UIS})

################# compiles resources files ############
SET(Calendars_RES ../resources/Calendars.qrc)
QT5_ADD_RESOURCES(Calendars_RES_CXX ${Calendars_RES})

ADD_LIBRARY(Calendars-static STATIC ${Calendars_SRCS} ${Calendars_RES_CXX} ${Calendars_UIS_H})
TARGET_LINK_LIBRARIES(Calendars-static Qt5::Core Qt5::Widgets)
SET_TARGET_PROPERTIES(Calendars-static PROPERTIES OUTPUT_NAME "Calendars")
SET_TARGET_PROPERTIES(Calendars-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
ADD_DEPENDENCIES(AllStaticPlugins Calendars-static)

SET_TARGET_PROPERTIES(Calendars-static PROPERTIES FOLDER "plugins/Calendars")
