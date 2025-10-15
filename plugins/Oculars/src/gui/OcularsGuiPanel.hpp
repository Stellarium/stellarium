/*
Oculars plug-in for Stellarium: graphical user interface widget
Copyright (C) 2011  Bogdan Marinov

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
*/

#ifndef OCULARSGUIPANEL_HPP
#define OCULARSGUIPANEL_HPP

#include <QGraphicsWidget>

class Oculars;
class StelButton;
class QGraphicsLinearLayout;
class QGraphicsProxyWidget;
class QLabel;
class QPushButton;
class QWidget;

//! A screen widget similar to InfoPanel. Contains controls and information.
//! @ingroup oculars
class OcularsGuiPanel : public QGraphicsWidget
{
	Q_OBJECT

public:
	OcularsGuiPanel(Oculars* ocularsPlugin,
			QGraphicsWidget * parent = Q_NULLPTR,
			Qt::WindowFlags wFlags = Qt::Widget);
	~OcularsGuiPanel() override;

public slots:
	//! Show only the controls used with an ocular overlay.
	void showOcularGui();
	//! Show only the controls used with a CCD overlay.
	void showCcdGui();
	//! Hide the controls, leaving only the button bar.
	void foldGui();

	void setFontSize(int size);

private slots:
	//! Update the position of the widget within the parent.
	//! Tied to the parent's geometryChanged() signal.
	void updatePosition();

	//! Updates the information shown when a sensor overlay is displayed
	void updateCcdControls();
	//! Updates the information that depends on the current lens
	void updateLensControls();
	//! Updates the information shown when an ocular overlay is displayed
	void updateOcularControls();
	//! Updates the information that depends on the current telescope.
	//! Called in both updateOcularControls() and updateCcdControls().
	void updateTelescopeControls();

	//! Sets the color scheme (day/night mode)
	//! @note: This is old code. @param schemeName is ignored.
	void setColorScheme(const QString& schemeName);

	void updateLayout();
	void updateRotationButtons();

private:
	Oculars* ocularsPlugin;

	//! This is actually SkyGui. Perhaps it should be more specific?
	QGraphicsWidget* parentWidget;

	QGraphicsLinearLayout* mainLayout;

	QGraphicsPathItem* borderPath;

	//! Mini-toolbar holding StelButtons
	QGraphicsWidget* buttonBar;
	QGraphicsWidget* ocularControls;
	QGraphicsWidget* lensControls;
	QGraphicsWidget* ccdControls;
	QGraphicsWidget* telescopeControls;

	//Mini-toolbar
	StelButton* buttonOcular;
	StelButton* buttonCrosshairs;
	StelButton* buttonCcd;
	StelButton* buttonTelrad;
	StelButton* buttonConfiguration;

	//Information display
	StelButton* prevOcularButton;
	StelButton* nextOcularButton;
	StelButton* prevTelescopeButton;
	StelButton* nextTelescopeButton;
	StelButton* prevCcdButton;
	StelButton* nextCcdButton;
	StelButton* prevLensButton;
	StelButton* nextLensButton;
	QGraphicsTextItem* fieldLensName;
	QGraphicsTextItem* fieldLensMultipler;
	QGraphicsTextItem* fieldOcularName;
	QGraphicsTextItem* fieldOcularFl;
	QGraphicsTextItem* fieldOcularAfov;
	QGraphicsTextItem* fieldCcdName;
	QGraphicsTextItem* fieldCcdDimensions;
	QGraphicsTextItem* fieldCcdBinning;
	QGraphicsTextItem* fieldCcdHScale;
	QGraphicsTextItem* fieldCcdVScale;
	QGraphicsTextItem* fieldCcdRotation;
	QGraphicsTextItem* fieldPrismRotation;
	QGraphicsTextItem* fieldTelescopeName;
	QGraphicsTextItem* fieldMagnification;
	QGraphicsTextItem* fieldExitPupil;
	QGraphicsTextItem* fieldTwilightFactor;
	QGraphicsTextItem* fieldRelativeBrightness;
	QGraphicsTextItem* fieldAdlerIndex;
	QGraphicsTextItem* fieldBishopIndex;
	QGraphicsTextItem* fieldFov;
	QGraphicsTextItem* fieldRayleighCriterion;
	QGraphicsTextItem* fieldDawesCriterion;
	QGraphicsTextItem* fieldAbbeyCriterion;
	QGraphicsTextItem* fieldSparrowCriterion;
	QGraphicsTextItem* fieldVisualResolution;
	QGraphicsTextItem* fieldLimitMagnitude;

	//Sensor frame rotation controls
	StelButton* rotateCcdMinus90Button = nullptr;
	StelButton* rotateCcdMinus15Button = nullptr;
	StelButton* rotateCcdMinus5Button = nullptr;
	StelButton* rotateCcdMinus1Button = nullptr;
	StelButton* resetCcdRotationButton = nullptr;
	StelButton* rotateCcdPlus1Button = nullptr;
	StelButton* rotateCcdPlus5Button = nullptr;
	StelButton* rotateCcdPlus15Button = nullptr;
	StelButton* rotateCcdPlus90Button = nullptr;
	StelButton* rotatePrismMinus90Button = nullptr;
	StelButton* rotatePrismMinus15Button = nullptr;
	StelButton* rotatePrismMinus5Button = nullptr;
	StelButton* rotatePrismMinus1Button = nullptr;
	StelButton* resetPrismRotationButton = nullptr;
	StelButton* rotatePrismPlus1Button = nullptr;
	StelButton* rotatePrismPlus5Button = nullptr;
	StelButton* rotatePrismPlus15Button = nullptr;
	StelButton* rotatePrismPlus90Button = nullptr;

	//! Sets the visibility of the ocular name label and the associated buttons.
	void setOcularControlsVisible(bool show);
	void setCcdControlsVisible(bool show);
	void setTelescopeControlsVisible(bool show);
	void setLensControlsVisible(bool show);
	//! Updates the positions of the buttons inside the button bar.
	void updateMainButtonsPositions();

	void setControlsColor(const QColor& color);
	void setControlsFont(const QFont& font);

	static QPixmap createPixmapFromText(const QString& text,
	                                    int width,
	                                    int height,
	                                    const QFont& font,
	                                    const QColor& textColor,
	                                    const QColor& backgroundColor = QColor(0,0,0,0));
};

#endif // OCULARSGUIPANEL_HPP
