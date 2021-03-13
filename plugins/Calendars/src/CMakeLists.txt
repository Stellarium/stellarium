INCLUDE_DIRECTORIES(. gui)

LINK_DIRECTORIES(${BUILD_DIR}/src)

SET(Calendars_SRCS
    Calendars.hpp
    Calendars.cpp
    gui/CalendarsDialog.hpp
    gui/CalendarsDialog.cpp
    gui/CalendarsInfoPanel.hpp
    gui/CalendarsInfoPanel.cpp
    Calendar.hpp
    Calendar.cpp
    EgyptianCalendar.hpp
    EgyptianCalendar.cpp
    ArmenianCalendar.hpp
    ArmenianCalendar.cpp
    ZoroastrianCalendar.hpp
    ZoroastrianCalendar.cpp
    JulianCalendar.hpp
    JulianCalendar.cpp
    RomanCalendar.hpp
    RomanCalendar.cpp
    OlympicCalendar.hpp
    OlympicCalendar.cpp
    GregorianCalendar.hpp
    GregorianCalendar.cpp
    ISOCalendar.hpp
    ISOCalendar.cpp
    CopticCalendar.hpp
    CopticCalendar.cpp
    EthiopicCalendar.hpp
    EthiopicCalendar.cpp
    IcelandicCalendar.hpp
    IcelandicCalendar.cpp
    IslamicCalendar.hpp
    IslamicCalendar.cpp
    HebrewCalendar.hpp
    HebrewCalendar.cpp
    OldHinduSolarCalendar.hpp
    OldHinduSolarCalendar.cpp
    OldHinduLuniSolarCalendar.hpp
    OldHinduLuniSolarCalendar.cpp
    MayaLongCountCalendar.hpp
    MayaLongCountCalendar.cpp
    MayaHaabCalendar.hpp
    MayaHaabCalendar.cpp
    MayaTzolkinCalendar.hpp
    MayaTzolkinCalendar.cpp
    AztecXihuitlCalendar.hpp
    AztecXihuitlCalendar.cpp
    AztecTonalpohualliCalendar.hpp
    AztecTonalpohualliCalendar.cpp
    BalinesePawukonCalendar.hpp
    BalinesePawukonCalendar.cpp
    FrenchArithmeticCalendar.hpp
    FrenchArithmeticCalendar.cpp
    PersianArithmeticCalendar.hpp
    PersianArithmeticCalendar.cpp
)

SET(Calendars_UIS
     gui/calendarsDialog.ui
)

QT5_WRAP_UI(Calendars_UIS_H ${Calendars_UIS})

################# compiles resources files ############
SET(Calendars_RES ../resources/Calendars.qrc)
QT5_ADD_RESOURCES(Calendars_RES_CXX ${Calendars_RES})

IF(ENABLE_TESTING)
    ADD_SUBDIRECTORY(test)
ENDIF(ENABLE_TESTING)

ADD_LIBRARY(Calendars-static STATIC ${Calendars_SRCS} ${Calendars_RES_CXX} ${Calendars_UIS_H})
TARGET_LINK_LIBRARIES(Calendars-static Qt5::Core Qt5::Widgets)
SET_TARGET_PROPERTIES(Calendars-static PROPERTIES OUTPUT_NAME "Calendars")
SET_TARGET_PROPERTIES(Calendars-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
ADD_DEPENDENCIES(AllStaticPlugins Calendars-static)

SET_TARGET_PROPERTIES(Calendars-static PROPERTIES FOLDER "plugins/Calendars")
