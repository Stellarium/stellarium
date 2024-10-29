/*
 * Stellarium OnlineQueries Plug-in
 *
 * Copyright (C) 2020-21 Georg Zotti
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

#ifndef STELWEBENGINEVIEW_HPP
#define STELWEBENGINEVIEW_HPP

#ifdef WITH_QTWEBENGINE

#include <QWebEngineView>

//! @class StelWebEngineView A trivial extension of QWebEngineView. 
//! This class provides a mouse button handler override which uses the mouse back/forward buttons for navigation.
//! To use in an .ui file in QCreator designer, add a QWebEngineView first,
//! then define it as "placeholder" for StelWebEngineView and import this header.

class StelWebEngineView: public QWebEngineView {
	Q_OBJECT
public:
	StelWebEngineView(QWidget *parent=nullptr);
protected:
	//! override to catch mouse button events.
	bool eventFilter(QObject *object, QEvent *event) override;
	//! improves mouse button navigation in the Web View by using back/forward buttons.
	void mouseReleaseEvent(QMouseEvent *e) override;
};

#else
#include <QTextBrowser>

//! @class StelWebEngineView Almost dummy class identical to @class QTextBrowser.
//! This is compiled on platforms without QWebEngine. It has QTextBrowser's setHtml(),
//! but any URL opening must be forwarded to the system's webbrowser.
class StelWebEngineView: public QTextBrowser {
	Q_OBJECT
public:
	StelWebEngineView(QWidget *parent=nullptr);
};
#endif
#endif
