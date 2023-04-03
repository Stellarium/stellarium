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

#ifndef SKYGUI_HPP
#define SKYGUI_HPP

#include "StelObject.hpp"

#include <QDebug>
#include <QGraphicsWidget>
#include <QGraphicsPixmapItem>

class QGraphicsSceneMouseEvent;
class QGraphicsTextItem;
class QTimeLine;
class StelButton;
class BottomStelBar;
class StelProgressController;

//! The information about the currently selected object
class InfoPanel : public QGraphicsTextItem
{
	public:
		//! Reads "gui/selected_object_info", etc from the configuration file.
		InfoPanel(QGraphicsItem* parent);
		~InfoPanel() override;
		void setInfoTextFilters(const StelObject::InfoStringGroup& aflags) {infoTextFilters=aflags;}
		const StelObject::InfoStringGroup& getInfoTextFilters(void) const {return infoTextFilters;}
		void setTextFromObjects(const QList<StelObjectP>&);
		const QString getSelectedText(void) const;

	private:
		StelObject::InfoStringGroup infoTextFilters;
		QGraphicsPixmapItem *infoPixmap; // Used when text rendering is buggy. Used when CLI option -t given.
};

//! The class managing the layout for button bars, selected object info and loading bars.
class SkyGui: public QGraphicsWidget
{
	Q_OBJECT

public:
	friend class StelGui;
	
	SkyGui(QGraphicsItem * parent=nullptr);
	//! Add a new progress bar in the lower right corner of the screen.
	//! When the progress bar is deleted with removeProgressBar() the layout is automatically rearranged.
	void addProgressBar(StelProgressController*);
	
	void init(class StelGui* stelGui);
	
	void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget* = nullptr) override;

	//! actually returns window width
	int getSkyGuiWidth() const;
	//! actually returns window height
	int getSkyGuiHeight() const;
	//! return height of bottom Bar when fully shown
	qreal getBottomBarHeight() const;
	//! return width of left Bar when fully shown
	qreal getLeftBarWidth() const;
	
protected:
	void resizeEvent(QGraphicsSceneResizeEvent* event) override;
	void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;
	QVariant itemChange(GraphicsItemChange change, const QVariant & value) override;

private slots:
	//! Load color scheme from the given ini file and section name
	void setStelStyle(const QString& style);
	
public slots:
	//! Update the position of the button bars in the main window
	//! GZ needed this public for interactive GUI scaling
	void updateBarsPos();

private:
	class StelBarsPath* buttonBarsPath;
	QTimeLine* animLeftBarTimeLine;
	QTimeLine* animBottomBarTimeLine;
	int lastBottomBarWidth;
	class LeftStelBar* leftBar;
	BottomStelBar* bottomBar;
	class InfoPanel* infoPanel;
	class StelProgressBarMgr* progressBarMgr;

	// The 2 auto hide buttons in the lower left corner
	StelButton* btHorizAutoHide;
	StelButton* btVertAutoHide;

	class CornerButtons* autoHidebts;

	bool autoHideBottomBar;
	bool autoHideLeftBar;
	
	StelGui* stelGui;
};

#endif // _SKYGUI_HPP
