/*
 * Copyright (C) 2019 Ruslan Kabatsayev
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

#ifndef STELLARIUM_SPLASH_SCREEN_HPP
#define STELLARIUM_SPLASH_SCREEN_HPP

#include <QThread>
#include <QApplication>
#include <QSplashScreen>

class SplashScreen
{
	class SplashScreenWidget : public QSplashScreen
	{
		bool painted=false;
		void paintEvent(QPaintEvent* e) override
		{
			QSplashScreen::paintEvent(e);
			painted=true;
		}
	public:
		SplashScreenWidget(QPixmap const& pixmap)
			: QSplashScreen(pixmap)
		{}
		void ensureFirstPaint() const
		{
			while(!painted)
			{
				QThread::msleep(1);
				qApp->processEvents();
			}
		}
	};

	static SplashScreenWidget* instance;

public:
	static void present();
	static void finish(QWidget* mainWindow);
	static void showMessage(QString const& message);
	static void clearMessage();
};

#endif
