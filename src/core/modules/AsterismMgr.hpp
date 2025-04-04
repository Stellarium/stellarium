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

#include <vector>
#include <QString>
#include <QStringList>
#include <QFont>

class StelToneReproducer;
class StarMgr;
class Asterism;
class StelSkyCulture;
class StelProjector;
class StelPainter;

//! @class AsterismMgr
//! Display and manage the asterisms.
//! It can display asterisms lines and names.
//! It also supports several different sky cultures.
class AsterismMgr : public StelObjectModule
{
	Q_OBJECT
	Q_PROPERTY(int fontSize					READ getFontSize						WRITE setFontSize						NOTIFY fontSizeChanged)
	Q_PROPERTY(Vec3f linesColor				READ getLinesColor					WRITE setLinesColor					NOTIFY linesColorChanged)
	Q_PROPERTY(bool linesDisplayed			READ getFlagLines					WRITE setFlagLines					NOTIFY linesDisplayedChanged)
	Q_PROPERTY(Vec3f rayHelpersColor			READ getRayHelpersColor				WRITE setRayHelpersColor				NOTIFY rayHelpersColorChanged)
	Q_PROPERTY(bool rayHelpersDisplayed		READ getFlagRayHelpers				WRITE setFlagRayHelpers				NOTIFY rayHelpersDisplayedChanged)
	Q_PROPERTY(Vec3f namesColor				READ getLabelsColor					WRITE setLabelsColor					NOTIFY namesColorChanged)
	Q_PROPERTY(bool namesDisplayed			READ getFlagLabels					WRITE setFlagLabels					NOTIFY namesDisplayedChanged)
	Q_PROPERTY(int asterismLineThickness		READ getAsterismLineThickness			WRITE setAsterismLineThickness			NOTIFY asterismLineThicknessChanged)
	Q_PROPERTY(int rayHelperThickness			READ getRayHelperThickness			WRITE setRayHelperThickness			NOTIFY rayHelperThicknessChanged)
	Q_PROPERTY(bool isolateAsterismSelected		READ getFlagIsolateAsterismSelected		WRITE setFlagIsolateAsterismSelected	NOTIFY isolateAsterismSelectedChanged)
	Q_PROPERTY(float namesFadeDuration		READ getLabelsFadeDuration			WRITE setLabelsFadeDuration			NOTIFY namesFadeDurationChanged)
	Q_PROPERTY(float linesFadeDuration			READ getLinesFadeDuration				WRITE setLinesFadeDuration			NOTIFY linesFadeDurationChanged)
	Q_PROPERTY(float rayHelpersFadeDuration	READ getRayHelpersFadeDuration		WRITE setRayHelpersFadeDuration		NOTIFY rayHelpersFadeDurationChanged)

public:
	//! Constructor
	AsterismMgr(StarMgr *stars);
	//! Destructor
	~AsterismMgr() override;

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	//! Initialize the AsterismMgr.
	//! Reads from the asterism parser object and updates the loading bar
	//! as asterism objects are loaded for the required sky culture.
	void init() override;

	//! Draw asterism lines, art, names and boundaries.
	void draw(StelCore* core) override;

	//! Updates time-varying state for each asterism.
	void update(double deltaTime) override;

	//! Return the value defining the order of call for the given action
	//! @param actionName the name of the action for which we want the call order
	//! @return the value defining the order. The closer to 0 the earlier the module's action will be called
	double getCallOrder(StelModuleActionName actionName) const override;

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in StelObjectModule class
	//QList<StelObjectP> searchAround(const Vec3d& v, double limitFov, const StelCore* core) const override;

	//! Return the matching asterism object's pointer if exists or Q_NULLPTR
	//! @param nameI18n The case in-sensitive asterism name
	StelObjectP searchByNameI18n(const QString& nameI18n) const override;

	//! Return the matching asterism if exists or Q_NULLPTR
	//! @param name The case in-sensitive standard program name (three letter abbreviation)
	StelObjectP searchByName(const QString& name) const override;

	StelObjectP searchByID(const QString &id) const override;

	QStringList listAllObjects(bool inEnglish) const override;
	QString getName() const override { return "Asterisms"; }
	QString getStelObjectType() const override;

	///////////////////////////////////////////////////////////////////////////
	// Properties setters and getters
public slots:	
	//! Define line color
	//! @param color The color of lines
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(1.0, 0.0, 0.0);
	//! AsterismMgr.setLinesColor(c.toVec3f());
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
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(1.0, 0.0, 0.0);
	//! AsterismMgr.setRayHelpersColor(c.toVec3f());
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
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(1.0, 0.0, 0.0);
	//! AsterismMgr.setLabelsColor(c.toVec3f());
	//! @endcode
	void setLabelsColor(const Vec3f& color);
	//! Get label color for names
	Vec3f getLabelsColor() const;

	//! Set whether asterism names will be displayed
	void setFlagLabels(const bool displayed);
	//! Set whether asterism names are displayed
	bool getFlagLabels(void) const;

	//! Set the font size used for asterism names display
	void setFontSize(const int newFontSize);
	//! Get the font size used for asterism names display
	int getFontSize() const;

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

	//! Set whether selected asterism must be displayed alone
	void setFlagIsolateAsterismSelected(const bool isolate);
	//! Get whether selected asterism is displayed alone
	bool getFlagIsolateAsterismSelected(void) const;

	//! Select the asterism by his English name. Calling this method will enable
	//! isolated selection for the asterisms if it is not enabled yet.
	//! @param englishName the English name of the asterism
	//! @code
	//! // example of usage in scripts: select the "Summer Triangle" asterism
	//! AsterismMgr.selectAsterism("Summer Triangle");
	//! @endcode
	void selectAsterism(const QString& englishName);

	//! Remove the asterism from list of selected asterisms by his English
	//! name. Calling this method will enable isolated selection for the asterisms
	//! if it is not enabled yet.
	//! @param englishName the English name of the asterism
	//! @code
	//! // example of usage in scripts: remove selection from the "Summer Triangle" asterism
	//! AsterismMgr.deselectAsterism("Summer Triangle");
	//! @endcode
	//! @note all asterisms will be hidden when list of selected asterisms will be empty
	void deselectAsterism(const QString& englishName);

	//! Remove asterisms from selected objects
	void deselectAsterisms(void);

	//! Select all asterisms
	void selectAllAsterisms(void);

	//! Set asterism lines fade duration in second
	void setLinesFadeDuration(const float duration);
	//! Get asterism lines fade duration in second
	float getLinesFadeDuration() const;

	//! Set asterism screen labels fade duration in second
	void setLabelsFadeDuration(const float duration);
	//! Get asterism screen labels fade duration in second
	float getLabelsFadeDuration() const;

	//! Set ray helpers fade duration in second
	void setRayHelpersFadeDuration(const float duration);
	//! Get ray helpers fade duration in second
	float getRayHelpersFadeDuration() const;

signals:
	void fontSizeChanged(const int newSize);
	void linesColorChanged(const Vec3f & color);
	void linesDisplayedChanged(const bool displayed);
	void namesColorChanged(const Vec3f & color);
	void namesDisplayedChanged(const bool displayed);
	void asterismLineThicknessChanged(int thickness);
	void rayHelpersColorChanged(const Vec3f & color);
	void rayHelpersDisplayedChanged(const bool displayed);
	void rayHelperThicknessChanged(int thickness);
	void isolateAsterismSelectedChanged(const bool isolate);
	void namesFadeDurationChanged(const float duration);
	void linesFadeDurationChanged(const float duration);
	void rayHelpersFadeDurationChanged(const float duration);

private slots:
	//! Loads new asterism data and art if the SkyCulture has changed.
	void updateSkyCulture(const StelSkyCulture& skyCulture);

	//! Update i18n names from English names according to current
	//! locale, and update font for locale.
	//! The translation is done using gettext with translated strings defined
	//! in translations.h
	void updateI18n();

	//! Limit the number of asterisms to draw based on selected asterisms.
	//! The selected objects changed, check if some asterism are selected and display the
	//! matching asterisms if isolateAsterismSelected mode is activated.
	//! @param action define whether to add to, replace, or remove from the existing selection
	void selectedObjectChange(StelModule::StelModuleSelectAction action);

	//! Toggle single asterism selection mode
	void switchSelectionMode();

private:
	//! Draw the asterism lines at the epoch given by the StelCore.
	void drawLines(StelPainter& sPainter, const StelCore* core) const;
	//! Draw the ray helpers at the epoch given by the StelCore.
	void drawRayHelpers(StelPainter& sPainter, const StelCore* core) const;
	//! Draw the asterism name labels.
	void drawNames(StelPainter& sPainter) const;
	//! Handle single and multi-asterism selections.
	void setSelectedAsterism(Asterism* a);
	//! Handle unselecting a single asterism.
	void unsetSelectedAsterism(Asterism* a);

	Asterism* findFromAbbreviation(const QString& abbreviation) const;

	std::vector<Asterism*> asterisms;
	std::vector<Asterism*> selected; // More than one can be selected at a time
	QFont asterFont;
	StarMgr* hipStarMgr;

	QString currentSkyCultureID;

	bool linesDisplayed;
	bool rayHelpersDisplayed;
	bool namesDisplayed;
	bool hasAsterism;
	bool isolateAsterismSelected; // true to pick individual asterisms.

	float linesFadeDuration;
	float namesFadeDuration;
	float rayHelpersFadeDuration;

	// Store the thickness of lines of the asterisms
	int asterismLineThickness;
	int rayHelperThickness;
};

#endif // ASTERISMMGR_HPP
