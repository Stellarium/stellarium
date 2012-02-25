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

StelGuiBase* StelMobileGuiPluginInterface::getStelGuiBase() const
{
	return new MobileGui();
}
Q_EXPORT_PLUGIN2(StelGui, StelMobileGuiPluginInterface)

MobileGui::MobileGui() : topLevelGraphicsWidget(NULL)
{

}

MobileGui::~MobileGui()
{

}

void MobileGui::init(QGraphicsWidget* topLevelGraphicsWidget, class StelAppGraphicsWidget* stelAppGraphicsWidget)
{
	this->topLevelGraphicsWidget = topLevelGraphicsWidget;
	StelGuiBase::init(topLevelGraphicsWidget, stelAppGraphicsWidget);
	/*
	QGraphicsScene* scene = myExistingGraphicsScene();
	 QDeclarativeEngine *engine = new QDeclarativeEngine;
	 QDeclarativeComponent component(engine, QUrl::fromLocalFile("myqml.qml"));
	 QGraphicsObject *object =
		 qobject_cast<QGraphicsObject *>(component.create());
	 scene->addItem(object);

	QDeclarativeEngine *engine = new QDeclarativeEngine;
	QDeclarativeComponent component(*/
}

//! Get a pointer on the info panel used to display selected object info
void MobileGui::setInfoTextFilters(const StelObject::InfoStringGroup& aflags)
{

}

const StelObject::InfoStringGroup& MobileGui::getInfoTextFilters() const
{

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
	return StelGuiBase::addGuiActions(actionName, text, shortCut, helpGroup, checkable, autoRepeat);
}

void MobileGui::forceRefreshGui()
{

}

//! Get the current visible status of the GUI.
bool MobileGui::getVisible() const
{
	return true;
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

}

//! Translate all texts to the new Locale.
void MobileGui::updateI18n()
{

}
