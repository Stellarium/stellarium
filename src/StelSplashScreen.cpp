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

#include "StelSplashScreen.hpp"
#include "StelFileMgr.hpp"
#include "StelUtils.hpp"

#include <QPainter>

SplashScreen::SplashScreenWidget* SplashScreen::instance;

static QPixmap makePixmap()
{
	QPixmap pixmap(StelFileMgr::findFile("data/splash.png"));
	return pixmap;
}

void SplashScreen::present()
{
	Q_ASSERT(!instance);
	instance=new SplashScreenWidget(makePixmap());
	instance->show();
	instance->ensureFirstPaint();
}

void SplashScreen::showMessage(QString const& message)
{
	Q_ASSERT(instance);
	instance->showMessage(message, Qt::AlignLeft|Qt::AlignBottom, Qt::white);
}

void SplashScreen::finish(QWidget* mainWindow)
{
	Q_ASSERT(instance);
	instance->hide();
	instance->finish(mainWindow);
	delete instance;
	instance=nullptr;
}

void SplashScreen::clearMessage()
{
	Q_ASSERT(instance);
	instance->clearMessage();
}

SplashScreen::SplashScreenWidget::SplashScreenWidget(QPixmap const& pixmap)
	: QSplashScreen(pixmap)
{
	splashFont.setPixelSize(11);

	// Fix concrete font family and rendering mode, so that even if default
	// application font changes, we still render app version in the same font
	// on each repaint.
	QFontInfo info(splashFont);
	splashFont.setFamily(info.family());
	splashFont.setHintingPreference(QFont::PreferFullHinting);
	splashFont.setStyleHint(QFont::AnyStyle, QFont::PreferAntialias);

	setFont(splashFont);
}

void SplashScreen::SplashScreenWidget::paintEvent(QPaintEvent* event)
{
	QSplashScreen::paintEvent(event);

	// Add version text
	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing);
	p.setPen(Qt::white);
#if defined(GIT_REVISION)
	QFontMetrics metrics(splashFont);
	p.drawText(QPointF(metrics.averageCharWidth(), 1.3*metrics.height()), QString("%1+ (v%2)").arg(StelUtils::getApplicationPublicVersion(), StelUtils::getApplicationVersion()));
#else
	QFont versionFont(splashFont);
	QString version = StelUtils::getApplicationPublicVersion();
	versionFont.setPixelSize(23);
	versionFont.setBold(true);
	QFontMetrics versionMetrics(versionFont);
	p.setFont(versionFont);
	p.drawText(365 - versionMetrics.horizontalAdvance(version), 293, version);
#endif

	painted=true;
}
