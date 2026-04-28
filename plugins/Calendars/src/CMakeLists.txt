INCLUDE_DIRECTORIES(. gui)

LINK_DIRECTORIES(${CMAKE_BINARY_DIR}/src)

SET(Calendars_SRCS
    Calendars.hpp
    Calendars.cpp
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
    RevisedJulianCalendar.hpp
    RevisedJulianCalendar.cpp
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
    NewHinduCalendar.hpp
    NewHinduCalendar.cpp
    NewHinduLunarCalendar.hpp
    NewHinduLunarCalendar.cpp
    AstroHinduLunarCalendar.hpp
    AstroHinduLunarCalendar.cpp
    AstroHinduSolarCalendar.hpp
    AstroHinduSolarCalendar.cpp
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
    FrenchAstronomicalCalendar.hpp
    FrenchAstronomicalCalendar.cpp
    PersianArithmeticCalendar.hpp
    PersianArithmeticCalendar.cpp
    PersianAstronomicalCalendar.hpp
    PersianAstronomicalCalendar.cpp
    TibetanCalendar.hpp
    TibetanCalendar.cpp
    BahaiArithmeticCalendar.hpp
    BahaiArithmeticCalendar.cpp
    BahaiAstronomicalCalendar.hpp
    BahaiAstronomicalCalendar.cpp
    ChineseCalendar.hpp
    ChineseCalendar.cpp
    JapaneseCalendar.hpp
    JapaneseCalendar.cpp
    KoreanCalendar.hpp
    KoreanCalendar.cpp
    VietnameseCalendar.hpp
    VietnameseCalendar.cpp
    gui/CalendarsInfoPanel.hpp
    gui/CalendarsInfoPanel.cpp
)
IF (STELLARIUM_GUI_MODE STREQUAL "Standard")
LIST(APPEND Calendars_SRCS
    gui/CalendarsDialog.hpp
    gui/CalendarsDialog.cpp
)

SET(Calendars_UIS
     gui/calendarsDialog.ui
)
ENDIF (STELLARIUM_GUI_MODE STREQUAL "Standard")

################# compiles resources files ############
SET(Calendars_RES ../resources/Calendars.qrc)
IF (${QT_VERSION_MAJOR} EQUAL "5")
     IF (STELLARIUM_GUI_MODE STREQUAL "Standard")
          QT5_WRAP_UI(Calendars_UIS_H ${Calendars_UIS})
     ENDIF()
     QT5_ADD_RESOURCES(Calendars_RES_CXX ${Calendars_RES})
ELSE()
     IF (STELLARIUM_GUI_MODE STREQUAL "Standard")
          QT_WRAP_UI(Calendars_UIS_H ${Calendars_UIS})
     ENDIF()
     QT_ADD_RESOURCES(Calendars_RES_CXX ${Calendars_RES})
ENDIF()

IF(ENABLE_TESTING)
    ADD_SUBDIRECTORY(test)
ENDIF(ENABLE_TESTING)

ADD_LIBRARY(Calendars-static STATIC ${Calendars_SRCS} ${Calendars_RES_CXX} ${Calendars_UIS_H})
if(ENABLE_PCH)
    # prepend a few includes to each cpp file.
     target_precompile_headers(Calendars-static PRIVATE
          $<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/JulianCalendar.hpp>
          $<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/GregorianCalendar.hpp>
          $<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/Calendar.hpp>)
     IF(CMAKE_VERSION VERSION_GREATER_EQUAL 3.19)
          SET(CMAKE_PCH_INSTANTIATE_TEMPLATES ON)
     ENDIF()
ENDIF(ENABLE_PCH)

TARGET_LINK_LIBRARIES(Calendars-static Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::OpenGL)
IF(ENABLE_SCRIPT_QML)
TARGET_LINK_LIBRARIES(Calendars-static Qt${QT_VERSION_MAJOR}::Qml)
ENDIF()
SET_TARGET_PROPERTIES(Calendars-static PROPERTIES OUTPUT_NAME "Calendars")
SET_TARGET_PROPERTIES(Calendars-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
SET_TARGET_PROPERTIES(Calendars-static PROPERTIES FOLDER "plugins/Calendars")
