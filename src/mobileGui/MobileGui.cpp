/*
 * Stellarium
 * Copyright (C) 2009 Fabien Chereau
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "MobileGui.hpp"
#include <QProgressBar>
#include <QtDeclarative/QDeclarativeEngine>
#include <QtDeclarative/QDeclarativeComponent>
#include <QDeclarativeContext>
#include <QGraphicsObject>
#include <QGraphicsWidget>
#include <QDebug>

#include <QGraphicsScene>
#include <QMessageBox>
#include <QAction>

#include <QGraphicsSceneMouseEvent>

#include <QApplication>
#include <QPoint>

#include "SystemDisplayInfo.hpp"
#include "UpdateSignallingItem.hpp"
#include "MobileImageProvider.hpp"
#include "MenuListModel.hpp"

#include "StelWrapper.hpp"

#include "StelModuleMgr.hpp"
#include "StelApp.hpp"
#include "StelAppGraphicsWidget.hpp"
#include "StelTranslator.hpp"
#include "StelCore.hpp"
#include "StelShortcutMgr.hpp"

#include "StelMainWindow.hpp"

//Actions
#include "NebulaMgr.hpp"
#include "StelMovementMgr.hpp"

StelGuiBase* StelMobileGuiPluginInterface::getStelGuiBase() const
{
	Q_INIT_RESOURCE(qmlResources);
	return new MobileGui();
}
Q_EXPORT_PLUGIN2(StelGui, StelMobileGuiPluginInterface)

MobileGui::MobileGui() : topLevelGraphicsWidget(NULL),
	engine(NULL), component(NULL), rootObject(NULL), displayInfo(NULL),
	stelWrapper(NULL)
{
    skyModel = new MenuListModel(this, N_("Sky"));
    viewModel = new MenuListModel(this, N_("View"));
    pluginsModel = new MenuListModel(this, N_("Plugins"));
}

MobileGui::~MobileGui()
{
	delete topLevelGraphicsWidget; topLevelGraphicsWidget = NULL;
	delete engine; engine = NULL;
	delete component; component = NULL;
	delete rootObject; rootObject = NULL;
	delete displayInfo; displayInfo = NULL;
	delete stelWrapper; stelWrapper = NULL;
}

void MobileGui::init(QGraphicsWidget* topLevelGraphicsWidget, class StelAppGraphicsWidget* stelAppGraphicsWidget)
{
    qRegisterMetaType<MenuListModel*>("MenuListModel*");

	this->topLevelGraphicsWidget = topLevelGraphicsWidget;
	StelGuiBase::init(topLevelGraphicsWidget, stelAppGraphicsWidget);

	UpdateSignallingItem * signaller = new UpdateSignallingItem(topLevelGraphicsWidget, this);
	Q_UNUSED(signaller)

	//QGraphicsScene* scene = myExistingGraphicsScene();

	engine = new QDeclarativeEngine();

	displayInfo = new SystemDisplayInfo();
	stelWrapper = new StelWrapper();

	engine->addImageProvider("mobileGui", new MobileImageProvider(displayInfo->dpiBucket()));

	engine->rootContext()->setContextProperty("displayInfo", displayInfo);
	engine->rootContext()->setContextProperty("stel", stelWrapper);
	engine->rootContext()->setContextProperty("baseGui", this);

	initActions();

    engine->rootContext()->setContextProperty("skyModel",  skyModel);
    engine->rootContext()->setContextProperty("viewModel",  viewModel);
    engine->rootContext()->setContextProperty("pluginsModel",  pluginsModel);

	component = new QDeclarativeComponent(engine, QUrl("qrc:/qml/MobileGui.qml"));

	if(component->isError())
	{
		qDebug() << "QDeclarativeComponent errors: " << component->errors();
		//an end-user should *never* see this, but make it _slightly_ friendly just in case
		topLevelGraphicsWidget->scene()->addWidget(new QMessageBox(QMessageBox::Critical, "Error", "An error has occured while loading. \nDetails: The QML failed to load. Check the log file for details."));
	}
	else
	{
		 qDebug() << "QDeclarativeComponent loaded in without complaint";
		 rootObject =
			 qobject_cast<QGraphicsObject *>(component->create());
		 guiSize.setWidth(topLevelGraphicsWidget->size().width());
		 guiSize.setHeight(topLevelGraphicsWidget->size().height());
		 rootObject->setProperty("width",guiSize.width());
		 rootObject->setProperty("height",guiSize.height());
		 rootObject->setParentItem(topLevelGraphicsWidget);
		 rootObject->setFocus();
		 //scene->addItem(object);
	}

	connectSignals();
}

//! Get a pointer on the info panel used to display selected object info
void MobileGui::setInfoTextFilters(const StelObject::InfoStringGroup& aflags)
{
	stelWrapper->setInfoTextFilters(aflags);
}

const StelObject::InfoStringGroup& MobileGui::getInfoTextFilters() const
{
	return stelWrapper->getInfoTextFilters();
}

//! Add a new progress bar in the lower right corner of the screen.
//! When the progress bar is deleted  the layout is automatically rearranged.
//! @return a pointer to the progress bar.
QProgressBar* MobileGui::addProgressBar()
{
	//return dummy progress bar for now;
	return new QProgressBar();
}

void MobileGui::forceRefreshGui()
{
	updateGui();
}

//! Get the current visible status of the GUI.
bool MobileGui::getVisible() const
{
	return rootObject->isVisible();
}
//! Show wether the Gui is currently used.
//! This can then be used to optimize the rendering to increase reactivity.
bool MobileGui::isCurrentlyUsed() const
{
	return true;
}

//! Show whether the GUI is visible.
//! @param b when true, GUI will be shown, else it will be hidden.
void MobileGui::setVisible(bool b)
{
	rootObject->setVisible(b);
}

//! Translate all texts to the new Locale.
void MobileGui::updateI18n()
{

}

void MobileGui::updateGui()
{
	if(guiSize != topLevelGraphicsWidget->size())
	{
		guiSize.setWidth(topLevelGraphicsWidget->size().width());
		guiSize.setHeight(topLevelGraphicsWidget->size().height());
		rootObject->setProperty("width",guiSize.width());
		rootObject->setProperty("height",guiSize.height());
	}

	StelCore* core = StelApp::getInstance().getCore();

    getGuiAction("actionDecrease_Time_Speed")->setChecked(core->getTimeRate()<-0.99*StelCore::JD_SECOND);
    getGuiAction("actionIncrease_Time_Speed")->setChecked(core->getTimeRate()>1.01*StelCore::JD_SECOND);
    getGuiAction("actionSet_Real_Time_Speed")->setChecked(core->getRealTimeSpeed());
    getGuiAction("actionReturn_To_Current_Time")->setChecked(core->getIsTimeNow());

	updated(); //signal the QML-side to update itself
}

void MobileGui::mousePress(int x, int y, int button, int buttons, int modifiers)
{
	stelAppGraphicsWidget->handleMousePress(QPointF(x, y), static_cast<Qt::MouseButton>(button),
											static_cast<Qt::MouseButtons>(buttons),
											static_cast<Qt::KeyboardModifiers>(modifiers));

}

void MobileGui::mouseRelease(int x, int y, int button, int buttons, int modifiers)
{
	stelAppGraphicsWidget->handleMouseRelease(QPointF(x, y), static_cast<Qt::MouseButton>(button),
											static_cast<Qt::MouseButtons>(buttons),
											static_cast<Qt::KeyboardModifiers>(modifiers));

}

void MobileGui::mouseMove(int x, int y, int button, int buttons, int modifiers)
{
	Q_UNUSED(button)
	Q_UNUSED(modifiers)
	stelAppGraphicsWidget->handleMouseMove(QPointF(x, y), static_cast<Qt::MouseButtons>(buttons));
}


void MobileGui::connectSignals()
{
	StelCore* core = StelApp::getInstance().getCore();

    //Sky input
    QObject::connect(rootObject, SIGNAL(fovChanged(qreal)), stelWrapper, SLOT(setFov(qreal)));

    QObject::connect(rootObject, SIGNAL(mousePressed(int, int, int, int, int)), this, SLOT(mousePress(int,int,int,int,int)));
    QObject::connect(rootObject, SIGNAL(mouseReleased(int, int, int, int, int)), this, SLOT(mouseRelease(int,int,int,int,int)));
    QObject::connect(rootObject, SIGNAL(mouseMoved(int, int, int, int, int)), this, SLOT(mouseMove(int,int,int,int,int)));

    //Action bar
    QObject::connect(getGuiAction("actionIncrease_Time_Speed"), SIGNAL(triggered()), core, SLOT(increaseTimeSpeed()));
    QObject::connect(getGuiAction("actionDecrease_Time_Speed"), SIGNAL(triggered()), core, SLOT(decreaseTimeSpeed()));
    QObject::connect(getGuiAction("actionSet_Real_Time_Speed"), SIGNAL(triggered()), core, SLOT(toggleRealTimeSpeed()));
    QObject::connect(getGuiAction("actionReturn_To_Current_Time"), SIGNAL(triggered()), core, SLOT(setTimeNow()));
    getGuiAction("actionReturn_To_Current_Time")->trigger(); //HACK: why is the app starting a few seconds behind real time?

    connect(getGuiAction("actionShow_Night_Mode"), SIGNAL(toggled(bool)), &StelApp::getInstance(), SLOT(setVisionModeNight(bool)));

    //Sky dialog
    NebulaMgr* nmgr = GETSTELMODULE(NebulaMgr);
    connect(getGuiAction("actionShow_Nebulas"), SIGNAL(toggled(bool)), nmgr, SLOT(setFlagHints(bool)));
}

void MobileGui::initActions()
{
    //Shortcuts are currently hardcoded. Ought they use the JSON shortcuts file like normal Stellarium?
    StelCore* core = StelApp::getInstance().getCore();

    //StelApp::getInstance().getStelShortcutManager()->loadShortcuts();

    StelShortcutMgr* shortcutMgr = StelApp::getInstance().getStelShortcutManager();

    QString group = N_("Date and Time");
    shortcutMgr->addGuiAction("actionDecrease_Time_Speed", true, N_("Decrease time speed"), "", "", group, true, false)->setChecked(core->getTimeRate()<-0.99*StelCore::JD_SECOND);
    shortcutMgr->addGuiAction("actionIncrease_Time_Speed", true, N_("Increase time speed"), "", "", group, true, false)->setChecked(core->getTimeRate()>1.01*StelCore::JD_SECOND);
    shortcutMgr->addGuiAction("actionSet_Real_Time_Speed", true, N_("Set normal time rate"), "", "", group, true, false)->setChecked(core->getRealTimeSpeed());
    shortcutMgr->addGuiAction("actionReturn_To_Current_Time", true, N_("Set time to now"), "", "", group, true, false)->setChecked(core->getIsTimeNow());
    shortcutMgr->addGuiAction("actionShow_Night_Mode", true, N_("Night mode"), "", "", group, true, false)->setChecked(StelApp::getInstance().getVisionModeNight());

    //Dialogs / screens (could be either one, really, depending on the screen's size)
    group = N_("Dialogs");
    shortcutMgr->addGuiAction("actionSky_Dialog", true, N_("Sky"), "", "", group, false, false);
    shortcutMgr->addGuiAction("actionView_Dialog", true, N_("View"), "", "", group, false, false);
    shortcutMgr->addGuiAction("actionPlugins_Dialog", true, N_("Plugins"), "", "", group, false, false);
    shortcutMgr->addGuiAction("actionDateTime_Dialog", true, N_("Date / Time"), "", "", group, false, false);
    shortcutMgr->addGuiAction("actionLocations_Dialog", true, N_("Location"), "", "", group, false, false);
    shortcutMgr->addGuiAction("actionSettings_Dialog", true, N_("Settings"), "", "", group, false, false);

    //Setup the Sky and View dialogs
    group = N_("SkyDialog");
    shortcutMgr->addGuiAction("actionShow_Nebulas", true, N_("Nebulas"), "", "", group, true);

    skyModel->addRow(getGuiAction("actionShow_Nebulas"), "image://mobileGui/showNebulas");
    viewModel->addRow(getGuiAction("actionShow_Nebulas"), "image://mobileGui/showNebulas");
}

QAction *MobileGui::getGuiAction(const QString &actionName)
{
    StelShortcutMgr* shortcutMgr = StelApp::getInstance().getStelShortcutManager();
    return shortcutMgr->getGuiAction(actionName);
}

void MobileGui::addPluginButton(const QString& imageSource, QAction* action)
{
    addGuiButton(imageSource, action, BG_Plugins);
}

void MobileGui::addPluginButton(const QString& imageSource, QString& actionName)
{
    addGuiButton(imageSource, actionName, BG_Plugins);
}

void MobileGui::addGuiButton(const QString& imageSource, QAction* action, ButtonGroup group)
{

}

void MobileGui::addGuiButton(const QString& imageSource, QString& actionName, ButtonGroup group)
{

}
