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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#ifndef _SKYGUI_HPP_
#define _SKYGUI_HPP_

#include "StelStyle.hpp"
#include "StelObject.hpp"

#include <QDebug>
#include <QGraphicsWidget>

class QGraphicsSceneMouseEvent;
class QAction;
class QGraphicsTextItem;
class QTimeLine;
class StelButton;
class BottomStelBar;


//! The informations about the currently selected object
class InfoPanel : public QGraphicsTextItem
{
	public:
		//! Reads "gui/selected_object_info", etc from the configuration file.
		//! @todo Bad idea to read from the configuration file in a constructor? --BM
		InfoPanel(QGraphicsItem* parent);
		void setInfoTextFilters(const StelObject::InfoStringGroup& aflags) {infoTextFilters=aflags;}
		const StelObject::InfoStringGroup& getInfoTextFilters(void) const {return infoTextFilters;}
		void setTextFromObjects(const QList<StelObjectP>&);
		const QString getSelectedText(void);

	private:
		StelObject::InfoStringGroup infoTextFilters;
};

//! The class managing the layout for button bars, selected object info and loading bars.
class SkyGui: public QGraphicsWidget
{
	Q_OBJECT

public:
	friend class StelGui;
	
	SkyGui(QGraphicsItem * parent=NULL);
	//! Add a new progress bar in the lower right corner of the screen.
	//! When the progress bar is deleted with removeProgressBar() the layout is automatically rearranged.
	//! @return a pointer to the progress bar
	class QProgressBar* addProgressBar();
	
	void init(class StelGui* stelGui);
	
	virtual void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget* = 0);
	
protected:
	virtual void resizeEvent(QGraphicsSceneResizeEvent* event);
	virtual void hoverMoveEvent(QGraphicsSceneHoverEvent* event);

private slots:
	//! Load color scheme from the given ini file and section name
	void setStelStyle(const QString& style);
	
	//! Update the position of the button bars in the main window
	void updateBarsPos();
	
private:
	class StelBarsPath* buttonBarPath;
	QTimeLine* animLeftBarTimeLine;
	QTimeLine* animBottomBarTimeLine;
	int lastButtonbarWidth;
	class LeftStelBar* winBar;
	BottomStelBar* buttonBar;
	class InfoPanel* infoPanel;
	class StelProgressBarMgr* progressBarMgr;

	// The 2 auto hide buttons in the lower left corner
	StelButton* btHorizAutoHide;
	StelButton* btVertAutoHide;

	class CornerButtons* autoHidebts;

	bool autoHideHorizontalButtonBar;
	bool autoHideVerticalButtonBar;
	
	StelGui* stelGui;
};

#endif // _SKYGUI_HPP_
