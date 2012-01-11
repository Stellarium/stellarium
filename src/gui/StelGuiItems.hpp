/*
 * Stellarium
 * Copyright (C) 2008 Fabien Chereau
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

#ifndef _STELGUIITEMS_HPP_
#define _STELGUIITEMS_HPP_

#include <QGraphicsPixmapItem>
#include <QGraphicsWidget>
#include <QDebug>

class QGraphicsSceneMouseEvent;
class QTimeLine;
class QAction;
class QGraphicsTextItem;
class QTimer;

// Progess bars in the lower right corner
class StelProgressBarMgr : public QGraphicsWidget
{
	Q_OBJECT
public:
	StelProgressBarMgr(QGraphicsItem* parent);
	class QProgressBar* addProgressBar();
};

// Buttons in the bottom left corner
class CornerButtons : public QObject, public QGraphicsItem
{
	Q_OBJECT
	Q_INTERFACES(QGraphicsItem);
public:
	CornerButtons(QGraphicsItem* parent=NULL);
	virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);
	virtual QRectF boundingRect() const;
	void setOpacity(double opacity);
private:
	mutable double lastOpacity;
};

//! A Button Graphicsitem for use in Stellarium's graphic widgets
class StelButton : public QObject, public QGraphicsPixmapItem
{
	friend class BottomStelBar;
	friend class LeftStelBar;
	Q_OBJECT
public:
	//! Constructor
	//! @param parent the parent item
	//! @param pixOn the pixmap to display when the button is toggled
	//! @param pixOff the pixmap to display when the button is not toggled
	//! @param pixHover a pixmap slowly blended when mouse is over the button
	//! @param action the associated action. Connections are automatically done with the signals if relevant.
	//! @param noBackground define whether the button background image have to be used
	StelButton(QGraphicsItem* parent, const QPixmap& pixOn, const QPixmap& pixOff,
			   const QPixmap& pixHover=QPixmap(),
			   QAction* action=NULL, bool noBackground=false);
	//! Constructor
	//! @param parent the parent item
	//! @param pixOn the pixmap to display when the button is toggled
	//! @param pixOff the pixmap to display when the button is not toggled
	//! @param pixNoChange the pixmap to display when the button state of a tristate is not changed
	//! @param pixHover a pixmap slowly blended when mouse is over the button
	//! @param action the associated action. Connections are automatically done with the signals if relevant.
	//! @param noBackground define whether the button background image have to be used
	//! @param isTristate define whether the button is a tristate or an on/off button
	StelButton(QGraphicsItem* parent, const QPixmap& pixOn, const QPixmap& pixOff, const QPixmap& pixNoChange,
			   const QPixmap& pixHover=QPixmap(),
			   QAction* action=NULL, bool noBackground=false, bool isTristate=true);
	//! Button states
	enum {ButtonStateOff = 0, ButtonStateOn = 1, ButtonStateNoChange = 2};

	//! Get whether the button is checked
	int isChecked() const {return checked;}

	//! Get the width of the button image.
	//! The width is based on pixOn.
	int getButtonPixmapWidth() const {return pixOn.width();}

	//! Set the button opacity
	void setOpacity(double v) {opacity=v; updateIcon();}

	//! Activate red mode for this button, i.e. will reduce the non red color component of the icon
	void setRedMode(bool b) {redMode=b; updateIcon();}

	//! Set the background pixmap of the button.
	//! A variant for night vision mode (pixBackgroundRed) is automatically
	//! generated from the new background.
	void setBackgroundPixmap(const QPixmap& newBackground);

	//! Transform the pixmap so that it look red for night vision mode
	static QPixmap makeRed(const QPixmap& p);

signals:
	//! Triggered when the button state changes
	void toggled(bool);
	//! Triggered when the button state changes
	void triggered();
	//! Emitted when the hover state change
	//! @param b true if the mouse entered the button
	void hoverChanged(bool b);

public slots:
	//! set whether the button is checked
	void setChecked(int b);
	void setChecked(bool b) { setChecked((int)b); }

protected:
	virtual void mousePressEvent(QGraphicsSceneMouseEvent* event);
	virtual void hoverEnterEvent(QGraphicsSceneHoverEvent* event);
	virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent* event);
	virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
private slots:
	void animValueChanged(qreal value);
private:
	void updateIcon();
	int toggleChecked(int);

	QPixmap pixOn;
	QPixmap pixOff;
	QPixmap pixNoChange;
	QPixmap pixHover;
	QPixmap pixBackground;

	QPixmap pixOnRed;
	QPixmap pixOffRed;
	QPixmap pixNoChangeRed;
	QPixmap pixHoverRed;
	QPixmap pixBackgroundRed;

	int checked;

	QTimeLine* timeLine;
	QAction* action;
	bool noBckground;
	bool isTristate_;
	double opacity;
	double hoverOpacity;

	bool redMode;
};

// The button bar on the left containing windows toggle buttons
class LeftStelBar : public QObject, public QGraphicsItem
{
	Q_OBJECT
	Q_INTERFACES(QGraphicsItem);
public:
	LeftStelBar(QGraphicsItem* parent);
	~LeftStelBar();
	virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);
	virtual QRectF boundingRect() const;
	void addButton(StelButton* button);
	QRectF boundingRectNoHelpLabel() const;
	//! Set the color for all the sub elements
	void setColor(const QColor& c);
	//! Activate red mode for the buttons, i.e. will reduce the non red color component of the icon
	void setRedMode(bool b);
private slots:
	//! Update the help label when a button is hovered
	void buttonHoverChanged(bool b);
private:
	QTimeLine* hideTimeLine;
	QGraphicsSimpleTextItem* helpLabel;
};

// The button bar on the bottom containing actions toggle buttons
class BottomStelBar : public QObject, public QGraphicsItem
{
	Q_OBJECT
	Q_INTERFACES(QGraphicsItem);
public:
	BottomStelBar(QGraphicsItem* parent, const QPixmap& pixLeft=QPixmap(), const QPixmap& pixRight=QPixmap(), const QPixmap& pixMiddle=QPixmap(), const QPixmap& pixSingle=QPixmap());
	virtual ~BottomStelBar();
	virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);
	virtual QRectF boundingRect() const;
	QRectF boundingRectNoHelpLabel() const;

	//! Add a button in a group in the button bar. Group are displayed in alphabetic order.
	//! @param button the button to add
	//! @param groupName the name of the button group to which the button belongs to. If the group doesn't exist yet, a new one is created.
	//! @param beforeActionName insert the button before the button associated to the given action. If the action doesn't exist, insert it at the end of the group.
	void addButton(StelButton* button, const QString& groupName="defaultGroup", const QString& beforeActionName="");
	//! Hide the button associated with the action of the passed name
	StelButton* hideButton(const QString& actionName);

	//! Set the margin at the left and right of a button group in pixels
	void setGroupMargin(const QString& groupName, int left, int right);

	//! Set the background of a group
	void setGroupBackground(const QString& groupName, const QPixmap& pixLeft=QPixmap(),
							const QPixmap& pixRight=QPixmap(), const QPixmap& pixMiddle=QPixmap(),
							const QPixmap& pixSingle=QPixmap());

	//! Set the color for all the sub elements
	void setColor(const QColor& c);

	//! Activate red mode for the buttons, i.e. will reduce the non red color component of the icon
	void setRedMode(bool b);

	//! Set whether time must be displayed in the bottom bar
	void setFlagShowTime(bool b) {flagShowTime=b;}
	//! Set whether location info must be displayed in the bottom bar
	void setFlagShowLocation(bool b) {flagShowLocation=b;}


private slots:
	//! Update the help label when a button is hovered
	void buttonHoverChanged(bool b);

private:
	void updateText(bool forceUpdatePos=false);
	void updateButtonsGroups();
	QRectF getButtonsBoundingRect() const;
	QGraphicsSimpleTextItem* location;
	QGraphicsSimpleTextItem* datetime;
	QGraphicsSimpleTextItem* fov;
	QGraphicsSimpleTextItem* fps;

	struct ButtonGroup
	{
		ButtonGroup() : leftMargin(0), rightMargin(0),
						pixBackgroundLeft(NULL), pixBackgroundRight(NULL),
						pixBackgroundMiddle(NULL), pixBackgroundSingle(NULL) {;}
		//! Elements of the group
		QList<StelButton*> elems;
		//! Left margin size in pixel
		int leftMargin;
		//! Right margin size in pixel
		int rightMargin;
		//! Background Images;
		QPixmap* pixBackgroundLeft;
		QPixmap* pixBackgroundRight;
		QPixmap* pixBackgroundMiddle;
		QPixmap* pixBackgroundSingle;
	};

	QMap<QString, ButtonGroup> buttonGroups;
	QPixmap pixBackgroundLeft;
	QPixmap pixBackgroundRight;
	QPixmap pixBackgroundMiddle;
	QPixmap pixBackgroundSingle;

	bool flagShowTime;
	bool flagShowLocation;

	QGraphicsSimpleTextItem* helpLabel;
};

// The path around the bottom left button bars
class StelBarsPath : public QGraphicsPathItem
{
	public:
		StelBarsPath(QGraphicsItem* parent);
		void updatePath(BottomStelBar* bot, LeftStelBar* lef);
		double getRoundSize() const {return roundSize;}
		void setBackgroundOpacity(double opacity);
	private:
		double roundSize;
};

#endif // _STELGUIITEMS_HPP_
