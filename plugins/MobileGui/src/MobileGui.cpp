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

#include "systemdisplayinfo.hpp"
#include "updatesignallingitem.hpp"
#include "skyinfowrapper.hpp"

StelGuiBase* StelMobileGuiPluginInterface::getStelGuiBase() const
{
	Q_INIT_RESOURCE(qmlResources);
	return new MobileGui();
}
Q_EXPORT_PLUGIN2(StelGui, StelMobileGuiPluginInterface)

MobileGui::MobileGui() : topLevelGraphicsWidget(NULL),
	engine(NULL), component(NULL), rootObject(NULL), displayInfo(NULL),
	skyInfoWrapper(NULL)
{

}

MobileGui::~MobileGui()
{
	delete topLevelGraphicsWidget; topLevelGraphicsWidget = NULL;
	delete engine; engine = NULL;
	delete component; component = NULL;
	delete rootObject; rootObject = NULL;
	delete displayInfo; displayInfo = NULL;
	delete skyInfoWrapper; skyInfoWrapper = NULL;
}

void MobileGui::init(QGraphicsWidget* topLevelGraphicsWidget, class StelAppGraphicsWidget* stelAppGraphicsWidget)
{
	this->topLevelGraphicsWidget = topLevelGraphicsWidget;
	StelGuiBase::init(topLevelGraphicsWidget, stelAppGraphicsWidget);

	UpdateSignallingItem * signaller = new UpdateSignallingItem(topLevelGraphicsWidget, this);
	Q_UNUSED(signaller)

	//QGraphicsScene* scene = myExistingGraphicsScene();

	engine = new QDeclarativeEngine();

	displayInfo = new SystemDisplayInfo();
	skyInfoWrapper = new SkyInfoWrapper();

	engine->rootContext()->setContextProperty("displayInfo", displayInfo);
	engine->rootContext()->setContextProperty("skyInfo", skyInfoWrapper);
	engine->rootContext()->setContextProperty("baseGui", this);


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
}

//! Get a pointer on the info panel used to display selected object info
void MobileGui::setInfoTextFilters(const StelObject::InfoStringGroup& aflags)
{
	skyInfoWrapper->setInfoTextFilters(aflags);
}

const StelObject::InfoStringGroup& MobileGui::getInfoTextFilters() const
{
	return skyInfoWrapper->getInfoTextFilters();
}

//! Add a new progress bar in the lower right corner of the screen.
//! When the progress bar is deleted  the layout is automatically rearranged.
//! @return a pointer to the progress bar.
QProgressBar* MobileGui::addProgressBar()
{
	//return dummy progress bar for now;
	return new QProgressBar();
}

//! Add a new action managed by the GUI. This method should be used to add new shortcuts to the program
//! @param actionName qt object name. Used as a reference for later uses
//! @param text the text to display when hovering, or in the help window
//! @param shortCut the qt shortcut to use
//! @param helpGroup hint on how to group the text in the help window
//! @param checkable whether the action should be checkable
//! @param autoRepeat whether the action should be autorepeated
QAction* MobileGui::addGuiActions(const QString& actionName, const QString& text, const QString& shortCut, const QString& helpGroup, bool checkable, bool autoRepeat)
{
	QAction* a = StelGuiBase::addGuiActions(actionName, text, shortCut, helpGroup, checkable, autoRepeat);

	//add the action to the engine's context so we can see it within the qml code
	engine->rootContext()->setContextProperty(QString("action_") + actionName, qobject_cast<QObject*>(a));

	return a;
}

QAction* MobileGui::getGuiActions(const QString &actionName)
{
	return StelGuiBase::getGuiActions(actionName);
}

/*QVariant MobileGui::getAction(const QString &actionName)
{
	QVariant action = engine->rootContext()->contextProperty("action_" + actionName);
	if(qobject_cast<QObject*>(action) == NULL)
	{
		qWarning(QString("Could not find guiAction ").append(actionName));
	}
}*/

//void MobileGui::addButton(...)
//add a new element to the listview corresponding to the button's category

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
	updated(); //signal the QML-side to update itself
}
