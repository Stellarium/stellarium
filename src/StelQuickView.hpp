/*
 * Stellarium
 * Copyright (C) 2015 Guillaume Chereau
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

#include <QQuickView>
#include <QQuickItem>

class QSettings;
class StelApp;

class StelQuickView : public QQuickView
{
	Q_OBJECT
public:
	StelQuickView();
	void init(class QSettings* conf);
	void deinit();
	//! Get the StelMainView singleton instance.
	static StelQuickView& getInstance() {Q_ASSERT(singleton); return *singleton;}
signals:
	void initialized();
protected slots:
	void paint();
	void synchronize();
	void showGui();
protected:
	void timerEvent(QTimerEvent* event) Q_DECL_OVERRIDE;
private:
	static StelQuickView* singleton;
	StelApp* stelApp;
	QSettings* conf;
	int initState;
};

// A special QQuickItem that can be used for mouse hovered area.  This
// works better than MouseArea because it handles overlapping areas correctly.
class HoverArea : public QQuickItem
{
	Q_OBJECT
	Q_PROPERTY(bool hovered MEMBER m_hovered NOTIFY hoveredChanged)
public:
	HoverArea(QQuickItem * parent=NULL);
protected:
	void timerEvent(QTimerEvent *event) Q_DECL_OVERRIDE;
signals:
	void hoveredChanged();
private:
	bool m_hovered;
};
