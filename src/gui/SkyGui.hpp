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

#include "StelStyle.hpp"
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

//! The informations about the currently selected object
class InfoPanel : public QGraphicsTextItem
{
	public:
		//! Reads "gui/selected_object_info", etc from the configuration file.
		//! @todo Bad idea to read from the configuration file in a constructor? --BM
		InfoPanel(QGraphicsItem* parent);
		~InfoPanel();
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
	
	SkyGui(QGraphicsItem * parent=Q_NULLPTR);
	//! Add a new progress bar in the lower right corner of the screen.
	//! When the progress bar is deleted with removeProgressBar() the layout is automatically rearranged.
	//! @return a pointer to the progress bar
	void addProgressBar(StelProgressController*);
	
	void init(class StelGui* stelGui);
	
	virtual void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget* = Q_NULLPTR) Q_DECL_OVERRIDE;

	int getSkyGuiWidth() const;
	int getSkyGuiHeight() const;
	qreal getBottomBarHeight() const; //!< return height of bottom Bar when fully shown
	qreal getLeftBarWidth() const;    //!< return width of left Bar when fully shown
	
protected:
	virtual void resizeEvent(QGraphicsSceneResizeEvent* event) Q_DECL_OVERRIDE;
	virtual void hoverMoveEvent(QGraphicsSceneHoverEvent* event) Q_DECL_OVERRIDE;
	virtual QVariant itemChange(GraphicsItemChange change, const QVariant & value) Q_DECL_OVERRIDE;

private slots:
	//! Load color scheme from the given ini file and section name
	void setStelStyle(const QString& style);
	
public slots:
	//! Update the position of the button bars in the main window
	//! GZ needed this public for interactive GUI scaling
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

#endif // _SKYGUI_HPP
