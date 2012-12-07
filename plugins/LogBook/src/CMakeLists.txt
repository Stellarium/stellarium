INCLUDE_DIRECTORIES(
	.
	gui
	gui/dataMappers
	gui/validators
	gui/widgets
	models
	${CMAKE_BINARY_DIR}
	${CMAKE_BINARY_DIR}/plugins/LogBook/src 
)

LINK_DIRECTORIES(/src)

SET(LogBook_SRCS
	LogBook.hpp
	LogBook.cpp
	LogBookCommon.hpp
	gui/LogBookConfigDialog.hpp
	gui/LogBookConfigDialog.cpp
	gui/ObservationsDialog.hpp
	gui/ObservationsDialog.cpp
	gui/SessionsDialog.hpp
	gui/SessionsDialog.cpp
	gui/StelDialogLogBook.hpp
	gui/StelDialogLogBook.cpp
	gui/TargetsDialog.hpp
	gui/TargetsDialog.cpp
	gui/dataMappers/BarlowsDataMapper.hpp
	gui/dataMappers/BarlowsDataMapper.cpp
	gui/dataMappers/FiltersDataMapper.hpp
	gui/dataMappers/FiltersDataMapper.cpp
	gui/dataMappers/ImagersDataMapper.hpp
	gui/dataMappers/ImagersDataMapper.cpp
	gui/dataMappers/ObserversDataMapper.hpp
	gui/dataMappers/ObserversDataMapper.cpp
	gui/dataMappers/OcularsDataMapper.hpp
	gui/dataMappers/OcularsDataMapper.cpp
	gui/dataMappers/OpticsDataMapper.hpp
	gui/dataMappers/OpticsDataMapper.cpp
	gui/dataMappers/SitesDataMapper.hpp
	gui/dataMappers/SitesDataMapper.cpp
	gui/validators/LimitingDoubleValidator.hpp
	gui/validators/LimitingDoubleValidator.cpp
	gui/validators/LimitingIntValidator.hpp
	gui/validators/LimitingIntValidator.cpp
	gui/validators/NonEmptyStringValidator.hpp
	gui/validators/NonEmptyStringValidator.cpp
	gui/widgets/LogBookTextEdit.hpp
	gui/widgets/LogBookTextEdit.cpp
	models/FieldConcatModel.hpp
	models/FieldConcatModel.cpp
)

### These source groups are only needed for grouping files in an IDE
SOURCE_GROUP(GUI FILES REGULAR_EXPRESSION .*Dialog*.+pp|ui$)
SOURCE_GROUP(DataMappers FILES REGULAR_EXPRESSION .*DataMapper.+pp)
SOURCE_GROUP(Models FILES REGULAR_EXPRESSION .*Model.+pp)
SOURCE_GROUP(Validators FILES REGULAR_EXPRESSION .*Validator*.+pp)


SET(LogBook_RES ../resources/LogBook.qrc)
QT4_ADD_RESOURCES(LogBook_RES_CXX ${LogBook_RES})

################# compiles .ui files ############
SET(LogBook_UIS
	gui/BarlowsWidget.ui
	gui/FiltersWidget.ui
	gui/ImagersWidget.ui
	gui/LogBookConfigDialog.ui
	gui/ObservationsDialog.ui
	gui/ObserversWidget.ui
	gui/OcularsWidget.ui
	gui/OpticsWidget.ui
	gui/SitesWidget.ui
	gui/SessionsDialog.ui
	gui/TargetsDialog.ui
)
QT4_WRAP_UI(LogBook_UIS_H ${LogBook_UIS})

SET(QT_USE_QTSQL TRUE)
INCLUDE(${QT_USE_FILE})

SET(extLinkerOption ${QT_LIBRARIES} ${OPENGL_LIBRARIES} ${ICONV_LIBRARIES} ${INTL_LIBRARIES})

############### For building the dynamic library ######################
IF(BUILD_DYNAMIC_PLUGINS)
	ADD_LIBRARY(LogBook 
		MODULE ${LogBook_SRCS} ${LogBook_RES_CXX} ${LogBook_UIS_H}
	)

	IF(APPLE)
		FIND_LIBRARY(OPENGL_LIBRARY OpenGL)
		MARK_AS_ADVANCED(OPENGL_LIBRARY)
		SET_TARGET_PROPERTIES(LogBook 
			PROPERTIES LINK_FLAGS "-undefined dynamic_lookup" SUFFIX ".dylib"
		)
	ENDIF(APPLE)

	IF(WIN32)
		SET_TARGET_PROPERTIES(LogBook 
			PROPERTIES LINK_FLAGS "-enable-runtime-pseudo-reloc -Wl,--allow-multiple-definition" 
		)
		SET(StelMain stelMain)
	ELSE(WIN32)
		SET(StelMain)
	ENDIF(WIN32)

	TARGET_LINK_LIBRARIES(LogBook ${StelMain} ${extLinkerOption})
	INSTALL(TARGETS LogBook DESTINATION "modules/${PACKAGE}")
ENDIF()

############### For building the static library ######################
IF(BUILD_STATIC_PLUGINS)
	ADD_LIBRARY(LogBook-static 
		STATIC ${LogBook_SRCS} ${LogBook_RES_CXX} ${LogBook_UIS_H}
	)
	SET_TARGET_PROPERTIES(LogBook-static PROPERTIES OUTPUT_NAME "LogBook")
	TARGET_LINK_LIBRARIES(LogBook-static ${StelMain} ${extLinkerOption})
	SET_TARGET_PROPERTIES(LogBook-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
	ADD_DEPENDENCIES(AllStaticPlugins LogBook-static)
ENDIF()

