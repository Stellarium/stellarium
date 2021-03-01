/*
 * Stellarium
 * Copyright (C) 2017 Alexander Wolf
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

#ifndef ASTERISMMGR_HPP
#define ASTERISMMGR_HPP

#include "StelObjectType.hpp"
#include "StelObjectModule.hpp"
#include "StelProjectorType.hpp"

#include <vector>
#include <QString>
#include <QStringList>
#include <QFont>

class StelToneReproducer;
class StarMgr;
class Asterism;
class StelProjector;
class StelPainter;

//! @class AsterismMgr
//! Display and manage the asterisms.
//! It can display asterisms lines and names.
//! It also supports several different sky cultures.
class AsterismMgr : public StelObjectModule
{
	Q_OBJECT
	Q_PROPERTY(float fontSize
		   READ getFontSize
		   WRITE setFontSize
		   NOTIFY fontSizeChanged)
	Q_PROPERTY(Vec3f linesColor
		   READ getLinesColor
		   WRITE setLinesColor
		   NOTIFY linesColorChanged)
	Q_PROPERTY(bool linesDisplayed
		   READ getFlagLines
		   WRITE setFlagLines
		   NOTIFY linesDisplayedChanged)
	Q_PROPERTY(Vec3f rayHelpersColor
		   READ getRayHelpersColor
		   WRITE setRayHelpersColor
		   NOTIFY rayHelpersColorChanged)
	Q_PROPERTY(bool rayHelpersDisplayed
		   READ getFlagRayHelpers
		   WRITE setFlagRayHelpers
		   NOTIFY rayHelpersDisplayedChanged)
	Q_PROPERTY(Vec3f namesColor
		   READ getLabelsColor
		   WRITE setLabelsColor
		   NOTIFY namesColorChanged)
	Q_PROPERTY(bool namesDisplayed
		   READ getFlagLabels
		   WRITE setFlagLabels
		   NOTIFY namesDisplayedChanged)
	Q_PROPERTY(int asterismLineThickness
		   READ getAsterismLineThickness
		   WRITE setAsterismLineThickness
		   NOTIFY asterismLineThicknessChanged)
	Q_PROPERTY(int rayHelperThickness
		   READ getRayHelperThickness
		   WRITE setRayHelperThickness
		   NOTIFY rayHelperThicknessChanged)

public:
	//! Constructor
	AsterismMgr(StarMgr *stars);
	//! Destructor
	virtual ~AsterismMgr();

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	//! Initialize the AsterismMgr.
	//! Reads from the asterism parser object and updates the loading bar
	//! as asterism objects are loaded for the required sky culture.
	virtual void init();

	//! Draw asterism lines, art, names and boundaries.
	virtual void draw(StelCore* core);

	//! Updates time-varying state for each asterism.
	virtual void update(double deltaTime);

	//! Return the value defining the order of call for the given action
	//! @param actionName the name of the action for which we want the call order
	//! @return the value defining the order. The closer to 0 the earlier the module's action will be called
	virtual double getCallOrder(StelModuleActionName actionName) const;

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in StelObjectModule class
	virtual QList<StelObjectP> searchAround(const Vec3d& v, double limitFov, const StelCore* core) const;

	//! Return the matching asterism object's pointer if exists or Q_NULLPTR
	//! @param nameI18n The case in-sensitive asterism name
	virtual StelObjectP searchByNameI18n(const QString& nameI18n) const;

	//! Return the matching asterism if exists or Q_NULLPTR
	//! @param name The case in-sensitive standard program name (three letter abbreviation)
	virtual StelObjectP searchByName(const QString& name) const;

	virtual StelObjectP searchByID(const QString &id) const;

	virtual QStringList listAllObjects(bool inEnglish) const;
	virtual QString getName() const { return "Asterisms"; }
	virtual QString getStelObjectType() const;

	///////////////////////////////////////////////////////////////////////////
	// Properties setters and getters
public slots:	
	//! Define line color
	//! @param color The color of lines
	//! @code
	//! // example of usage in scripts
	//! AsterismMgr.setLinesColor(Vec3f(1.0,0.0,0.0));
	//! @endcode
	void setLinesColor(const Vec3f& color);
	//! Get line color
	Vec3f getLinesColor() const;

	//! Set whether asterism lines will be displayed
	void setFlagLines(const bool displayed);
	//! Get whether asterism lines are displayed
	bool getFlagLines(void) const;

	//! Define ray helper color
	//! @param color The color of ray helpers
	//! @code
	//! // example of usage in scripts
	//! AsterismMgr.setRayHelpersColor(Vec3f(1.0,0.0,0.0));
	//! @endcode
	void setRayHelpersColor(const Vec3f& color);
	//! Get ray helper color
	Vec3f getRayHelpersColor() const;

	//! Set whether ray helpers will be displayed
	void setFlagRayHelpers(const bool displayed);
	//! Get whether ray helpers are displayed
	bool getFlagRayHelpers(void) const;

	//! Set label color for names
	//! @param color The color of labels
	//! @code
	//! // example of usage in scripts
	//! AsterismMgr.setLabelsColor(Vec3f(1.0,0.0,0.0));
	//! @endcode
	void setLabelsColor(const Vec3f& color);
	//! Get label color for names
	Vec3f getLabelsColor() const;

	//! Set whether asterism names will be displayed
	void setFlagLabels(const bool displayed);
	//! Set whether asterism names are displayed
	bool getFlagLabels(void) const;

	//! Set the font size used for asterism names display
	void setFontSize(const float newFontSize);
	//! Get the font size used for asterism names display
	float getFontSize() const;

	//! Set the thickness of lines of the asterisms
	//! @param thickness of line in pixels
	void setAsterismLineThickness(const int thickness);
	//! Get the thickness of lines of the asterisms
	int getAsterismLineThickness() const { return asterismLineThickness; }

	//! Set the thickness of ray helpers of the asterisms
	//! @param thickness of ray helper in pixels
	void setRayHelperThickness(const int thickness);
	//! Get the thickness of ray helper of the asterisms
	int getRayHelperThickness() const { return rayHelperThickness; }

	//! @return true if asterism lines is defined
	bool isLinesDefined() { return hasAsterism; }

signals:
	void fontSizeChanged(const float newSize) const;
	void linesColorChanged(const Vec3f & color) const;
	void linesDisplayedChanged(const bool displayed) const;
	void namesColorChanged(const Vec3f & color) const;
	void namesDisplayedChanged(const bool displayed) const;
	void asterismLineThicknessChanged(int thickness) const;
	void rayHelpersColorChanged(const Vec3f & color) const;
	void rayHelpersDisplayedChanged(const bool displayed) const;
	void rayHelperThicknessChanged(int thickness) const;

private slots:
	//! Loads new asterism data and art if the SkyCulture has changed.
	//! @param skyCultureDir the name of the directory containing the sky culture to use.
	void updateSkyCulture(const QString& skyCultureDir);

	//! Update i18n names from English names according to current
	//! locale, and update font for locale.
	//! The translation is done using gettext with translated strings defined
	//! in translations.h
	void updateI18n();

private:
	//! Read asterism names from the given file.
	//! @param namesFile Name of the file containing the asterism names
	//!        in a format consisting of abbreviation and translatable english name.
	//! @note The abbreviation must occur in the lines file loaded first in @name loadLines()!
	void loadNames(const QString& namesFile);

	//! Load asterism line shapes from data files.
	//! @param fileName The name of the asterism data file
	//! @note The abbreviation used in @param filename is required for cross-identifying translatable names in @name loadNames():
	void loadLines(const QString& fileName);

	//! Draw the asterism lines at the epoch given by the StelCore.
	void drawLines(StelPainter& sPainter, const StelCore* core) const;
	//! Draw the ray helpers at the epoch given by the StelCore.
	void drawRayHelpers(StelPainter& sPainter, const StelCore* core) const;
	//! Draw the asterism name labels.
	void drawNames(StelPainter& sPainter) const;

	Asterism* isStarIn(const StelObject *s) const;
	Asterism* findFromAbbreviation(const QString& abbreviation) const;

	//Constellation* isStarIn(const StelObject *s) const;
	//Constellation* findFromAbbreviation(const QString& abbreviation) const;
	std::vector<Asterism*> asterisms;
	QFont asterFont;
	StarMgr* hipStarMgr;

	QString lastLoadedSkyCulture;	// Store the last loaded sky culture directory name
	QString currentSkyCultureID;

	bool linesDisplayed;
	bool rayHelpersDisplayed;
	bool namesDisplayed;
	bool hasAsterism;

	// Store the thickness of lines of the asterisms
	int asterismLineThickness;
	int rayHelperThickness;
};

#endif // ASTERISMMGR_HPP
