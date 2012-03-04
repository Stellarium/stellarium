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

StelGuiBase* StelMobileGuiPluginInterface::getStelGuiBase() const
{
	Q_INIT_RESOURCE(qmlResources);
	return new MobileGui();
}
Q_EXPORT_PLUGIN2(StelGui, StelMobileGuiPluginInterface)

MobileGui::MobileGui() : topLevelGraphicsWidget(NULL),
	engine(NULL), component(NULL), rootObject(NULL), displayInfo(NULL)
{

}

MobileGui::~MobileGui()
{
	delete topLevelGraphicsWidget; topLevelGraphicsWidget = NULL;
	delete engine; engine = NULL;
	delete component; component = NULL;
	delete rootObject; rootObject = NULL;
	delete displayInfo; displayInfo = NULL;
}

void MobileGui::init(QGraphicsWidget* topLevelGraphicsWidget, class StelAppGraphicsWidget* stelAppGraphicsWidget)
{
	this->topLevelGraphicsWidget = topLevelGraphicsWidget;
	StelGuiBase::init(topLevelGraphicsWidget, stelAppGraphicsWidget);

	//QGraphicsScene* scene = myExistingGraphicsScene();

	engine = new QDeclarativeEngine();

	displayInfo = new SystemDisplayInfo();
	engine->rootContext()->setContextProperty("DisplayInfo", displayInfo);


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
		 rootObject->setProperty("width",topLevelGraphicsWidget->size().width());
		 rootObject->setProperty("height",topLevelGraphicsWidget->size().height());
		 rootObject->setParentItem(topLevelGraphicsWidget);
		 //scene->addItem(object);

	}
}

//! Get a pointer on the info panel used to display selected object info
void MobileGui::setInfoTextFilters(const StelObject::InfoStringGroup& aflags)
{
	infoTextFilters = aflags;
}

const StelObject::InfoStringGroup& MobileGui::getInfoTextFilters() const
{
	return infoTextFilters;
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

//void MobileGui::addButton(...)
//add a new element to the listview corresponding to the button's category

void MobileGui::forceRefreshGui()
{

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
