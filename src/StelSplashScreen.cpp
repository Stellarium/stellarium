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
#include <cmath>

SplashScreen::SplashScreenWidget* SplashScreen::instance;
using TextHorizMetrics = SplashScreenTextHorizMetrics;

// This class is used in getRealTextWidthAndOffset, see the rationale in its docs.
class SplashTextHolder : public QWidget
{
	QString text;
	QPoint textPos;
public:
	SplashTextHolder(QWidget* parent = nullptr)
	 : QWidget(parent)
	{
		setAttribute(Qt::WA_NoSystemBackground);
	}
	void setText(const QString& text)
	{
		this->text = text;
		const auto rect = QFontMetrics(font()).tightBoundingRect(text);
		resize(rect.size() * 1.5);
	}
	QPoint lastTextPos() const { return textPos; }
protected:
	void paintEvent(QPaintEvent*) override
	{
		QPainter p(this);
		p.setFont(font());
		p.fillRect(rect(), Qt::black);
		p.setPen(Qt::white);
		const auto textRect = QFontMetrics(font()).tightBoundingRect(text);
		textPos = {(width() - textRect.width()) / 2,
		           (height() + textRect.height()) / 2};
		p.drawText(textPos, text);
	}
};

namespace
{

constexpr int BASE_FONT_SIZE = 11;
constexpr int BASE_PIXMAP_HEIGHT = 365;

const QString titleText = "Stellarium";
const QString subtitleText = "stellarium.org";

// QFontMetrics::tightBoundingRect appears to be unreliable for different fonts
// on Ubuntu 20.04 with Qt 6.4.1. This function tries to find the actual width
// of the text, and not just the one rendered on a QImage, but the one that a
// QWidget would paint, the difference being in subpixel smoothing being off
// and on respectively, which appears to affect the real width of the text.
TextHorizMetrics getRealTextWidthAndOffset(SplashTextHolder& h, const QFont& font, const QString& text)
{
	const auto rect = QFontMetrics(font).tightBoundingRect(text);
	QImage img(rect.size() * 1.5, QImage::Format_Grayscale8);
	h.setFont(font);
	h.setText(text);
	img = h.grab()
	       .toImage()
	       .convertToFormat(QImage::Format_Grayscale8)
	       .scaled(img.width(), 1, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
	const uchar*const data = img.constBits();
	int leftMargin = 0, rightMargin = 0;
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
	const int step = 1;
#else
	// Qt5's conversion to Grayscale8 appears to keep pixel data
	// in {gray,gray,gray,255} format instead of simple {gray}.
	// We'll compute the step indirectly, just in case something
	// works even more unexpectedly.
	const int step = img.bytesPerLine() / img.width(); // expected to be 4
#endif
	while(leftMargin < img.width() && data[leftMargin * step] == 0)
		++leftMargin;
	while(rightMargin < img.width() && data[(img.width() - 1 - rightMargin) * step] == 0)
		++rightMargin;
	if(rightMargin + leftMargin >= img.width())
	{
		qWarning() << "getRealTextWidthAndOffset() failed: the image seems to be empty";
		return {0, rect.width()};
	}
	TextHorizMetrics metrics;
	metrics.shift = leftMargin - h.lastTextPos().x();
	metrics.width = img.width() - leftMargin - rightMargin;
	return metrics;
}

void setBestFontStretch(SplashTextHolder& h, QFont& font, const QString& text,
                        const int desiredWidth)
{
	int min = 1, max = 200;
	int minWidth = 0, maxWidth = INT_MAX;
	while(max - min > 1)
	{
		const int curr = (min + max) / 2;
		font.setStretch(curr);
		const auto metrics = getRealTextWidthAndOffset(h, font, text);
		if(metrics.width > desiredWidth)
		{
			max = curr;
			maxWidth = metrics.width;
		}
		else if(metrics.width < desiredWidth)
		{
			min = curr;
			minWidth = metrics.width;
		}
		else // if exactly equal
		{
			return;
		}
	}

	if(min == max) return;

	if(std::abs(minWidth - desiredWidth) < std::abs(maxWidth - desiredWidth))
		font.setStretch(min);
	else
		font.setStretch(max);
}

}

void SplashScreen::present(const double sizeRatio)
{
	Q_ASSERT(!instance);
	instance=new SplashScreenWidget(sizeRatio);
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

SplashScreen::SplashScreenWidget::SplashScreenWidget(const double sizeRatio)
	: textHolder(new SplashTextHolder)
	, titleFont("DejaVu Sans")
	, sizeRatio(sizeRatio)
{
	splashFont.setPixelSize(std::lround(BASE_FONT_SIZE * sizeRatio));

	// Fix concrete font family and rendering mode, so that even if default
	// application font changes, we still render app version in the same font
	// on each repaint.
	QFontInfo info(splashFont);
	splashFont.setFamily(info.family());
	splashFont.setHintingPreference(QFont::PreferFullHinting);
	splashFont.setStyleHint(QFont::AnyStyle, QFont::PreferAntialias);

	setFont(splashFont);

	titleFont.setPixelSize(55 * sizeRatio);
	titleFont.setWeight(QFont::Bold);
	titleFont.setHintingPreference(QFont::PreferFullHinting);
	setBestFontStretch(*textHolder, titleFont, titleText, sizeRatio * 290);
	titleHM = getRealTextWidthAndOffset(*textHolder, titleFont, titleText);

	subtitleFont = titleFont;
	subtitleFont.setWeight(QFont::Normal);
	subtitleFont.setPixelSize(18 * sizeRatio);
	subtitleHM = getRealTextWidthAndOffset(*textHolder, subtitleFont, subtitleText);

	versionFont = splashFont;
	versionFont.setPixelSize(23 * sizeRatio);
	versionFont.setBold(true);
	const auto lastTitleLetterBR = QFontMetrics(titleFont).tightBoundingRect(titleText.back());
	const auto versionText = StelUtils::getApplicationPublicVersion();
	setBestFontStretch(*textHolder, versionFont, versionText,
	                   lastTitleLetterBR.width());
	versionHM = getRealTextWidthAndOffset(*textHolder, versionFont, versionText);

	setPixmap(makePixmap(sizeRatio));
}

void SplashScreen::SplashScreenWidget::paintEvent(QPaintEvent* event)
{
	QSplashScreen::paintEvent(event);

	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing);
	p.setPen(Qt::white);

	// Add branding text: "Stellarium" in large bold font centered horizontally,
	// and "stellarium.org" under it in a smaller font, both aligned to each
	// other on the right side.
	p.setPen(Qt::white);
	const QFontMetrics titleFM(titleFont);
	auto titleBR = titleFM.tightBoundingRect(titleText);
	titleBR.setWidth(titleHM.width);
	const QPoint titlePos((width() - titleBR.width()) / 2,
	                      std::lround(height() * 0.89));
	p.setFont(titleFont);
	p.drawText(titlePos, titleText);

	const QFontMetrics subtitleFM(subtitleFont);
	auto subtitleBR = subtitleFM.tightBoundingRect(subtitleText);
	subtitleBR.setWidth(subtitleHM.width);
	subtitleBR.translate(subtitleHM.shift - subtitleBR.x(), 0);
	p.setFont(subtitleFont);
	const int subtitleTextX = titlePos.x() +
		(titleHM.shift - subtitleHM.shift) +
		(titleHM.width - subtitleHM.width);
	const int subtitleTextY = titlePos.y() + titleBR.bottom() + subtitleBR.height();
	p.drawText(QPoint(subtitleTextX, subtitleTextY), subtitleText);

	// Add version text
#if defined(GIT_REVISION)
	QFontMetrics metrics(splashFont);
	p.setFont(font());
	const auto verStr = QString("%1+ (v%2)").arg(StelUtils::getApplicationPublicVersion(),
	                                             StelUtils::getApplicationVersion());
	p.drawText(QPointF(metrics.averageCharWidth(), 1.3*metrics.height()), verStr);
#else
	QString version = StelUtils::getApplicationPublicVersion();
	QFontMetrics versionMetrics(versionFont);
	p.setFont(versionFont);
	const int versionTextX = titlePos.x() +
		(titleHM.shift - versionHM.shift) +
		(titleHM.width - versionHM.width);
	p.drawText(versionTextX, 0.8 * height(), version);
#endif

	painted=true;
}

QPixmap SplashScreen::SplashScreenWidget::makePixmap(const double sizeRatio)
{
	QPixmap pixmap(StelFileMgr::findFile("data/splash.png"));
	pixmap = pixmap.scaledToHeight(std::lround(BASE_PIXMAP_HEIGHT * sizeRatio),
	                               Qt::SmoothTransformation);
	return pixmap;
}
