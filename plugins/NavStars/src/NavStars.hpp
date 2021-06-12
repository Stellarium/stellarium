/*
 * Navigational Stars plug-in
 * Copyright (C) 2014-2016 Alexander Wolf
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

#ifndef NAVSTARS_HPP
#define NAVSTARS_HPP

#include "StelFader.hpp"
#include "StelModule.hpp"
#include "StelObject.hpp" // For StelObjectP
#include "StelTexture.hpp"

#include <QSettings>

#include "NavStarsCalculator.hpp"

class StelButton;
class StelPainter;
class StelPropertyMgr;
class NavStarsWindow;

/*! @defgroup navigationalStars Navigational Stars Plug-in
@{
The Navigational Stars plugin marks the 58 navigational stars of The
Nautical Almanac and the 2102-D Rude Star Finder on the sky. Alternatively,
the French, German, and Russian selection of navigational stars are also
available.

The NavStars class is the main class of the plug-in. It manages the list of
navigational stars and manipulate show/hide markers of them. Markers
are not objects!

The plugin is also an example of a custom plugin that just marks existing stars.

<b>Configuration</b>

The plug-ins' configuration data is stored in Stellarium's main configuration
file (section [NavigationalStars]).

@}
*/

//! @class NavStars
//! Main class of the %Navigational Stars plugin.
//! @author Alexander Wolf
//! @author Andy Kirkham
//! @ingroup navigationalStars
class NavStars : public StelModule
{
	Q_OBJECT
	Q_ENUMS(NavigationalStarsSet)
	Q_PROPERTY(bool navStarsVisible		READ getNavStarsMarks		WRITE setNavStarsMarks		NOTIFY navStarsMarksChanged)
	Q_PROPERTY(bool displayAtStartup	READ getEnableAtStartup		WRITE setEnableAtStartup	NOTIFY enableAtStartupChanged)
	Q_PROPERTY(bool highlightWhenVisible	READ getHighlightWhenVisible	WRITE setHighlightWhenVisible   NOTIFY highlightWhenVisibleChanged)
	Q_PROPERTY(bool limitInfoToNavStars	READ getLimitInfoToNavStars	WRITE setLimitInfoToNavStars	NOTIFY limitInfoToNavStarsChanged)
	Q_PROPERTY(bool upperLimb		READ getUpperLimb		WRITE setUpperLimb		NOTIFY upperLimbChanged)
	Q_PROPERTY(bool tabulatedDisplay	READ getTabulatedDisplay	WRITE setTabulatedDisplay	NOTIFY tabulatedDisplayChanged)
	Q_PROPERTY(bool showExtraDecimals	READ getShowExtraDecimals	WRITE setShowExtraDecimals	NOTIFY showExtraDecimalsChanged)
	Q_PROPERTY(bool useUTCTime		READ getFlagUseUTCTime		WRITE setFlagUseUTCTime		NOTIFY flagUseUTCTimeChanged)
public:	
	//! @enum NavigationalStarsSet
	//! Available sets of navigational stars
	enum NavigationalStarsSet
	{
		AngloAmerican,	//!< Anglo-American set (The Nautical Almanac)
		French,		//!< French set (Ephémérides Nautiques)
		Russian,		//!< Russian set (Морской астрономический ежегодник)
		German		//!< German set (Nautisches Jahrbuch)
	};

	NavStars();
	virtual ~NavStars();

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	virtual void init();
	virtual void deinit();
	virtual void update(double deltaTime);
	virtual void draw(StelCore* core);
	virtual double getCallOrder(StelModuleActionName actionName) const;
	virtual bool configureGui(bool show);

	//! Set up the plugin with default values.  This means clearing out the NavigationalStars section in the
	//! main config.ini (if one already exists), and populating it with default values.
	void restoreDefaultConfiguration(void);

	//! Read (or re-read) settings from the main config file.  This will be called from init and also
	//! when restoring defaults (i.e. from the configuration dialog / restore defaults button).
	void loadConfiguration(void);

	//! Save the settings to the main configuration file.
	void saveConfiguration(void);

	void populateNavigationalStarsSet(void);

	QList<int> getStarsNumbers(void) { return starNumbers; }

public slots:
	//! Set flag of displaying markers of the navigational stars
	//! Emits navStarsMarksChanged() if the value changes.
	void setNavStarsMarks(const bool b);
	//! Get flag of displaying markers of the navigational stars
	bool getNavStarsMarks(void) const;

	void setEnableAtStartup(bool b);
	bool getEnableAtStartup(void) const { return enableAtStartup; }

	void setHighlightWhenVisible(bool b);
	bool getHighlightWhenVisible(void) const { return highlightWhenVisible; }

	void setLimitInfoToNavStars(bool b);
	bool getLimitInfoToNavStars(void) const { return limitInfoToNavStars; }

	void setUpperLimb(bool b);
	bool getUpperLimb(void) const { return upperLimb; }

	void setTabulatedDisplay(bool b);
	bool getTabulatedDisplay(void) const { return tabulatedDisplay; }

	void setShowExtraDecimals(bool b);
	bool getShowExtraDecimals(void) const { return NavStarsCalculator::useExtraDecimals; }

	void setFlagUseUTCTime(bool b);
	bool getFlagUseUTCTime(void) const { return useUTCTime; }

	//! Set the set of navigational stars
	void setCurrentNavigationalStarsSet(NavigationalStarsSet nsset)
	{
		currentNSSet = nsset;
	}
	//! Get the set of navigational stars
	NavigationalStarsSet getCurrentNavigationalStarsSet() const
	{
		return currentNSSet;
	}
	//! Get the key of current set of navigational stars
	QString getCurrentNavigationalStarsSetKey(void) const;
	QString getCurrentNavigationalStarsSetDescription(void) const;
	//! Set the set of navigational stars from its key
	void setCurrentNavigationalStarsSetKey(QString key);

	//! For the currently select object add the extraString info
	//! in a format that matches the Nautical Almanac.
	//REMOVE!void extraInfoStrings(const QMap<QString, double>& data, QMap<QString, QString>& strings, QString extraText = "");

	//! Adds StelObject::ExtraInfo for selected object.
	void addExtraInfo(StelCore* core);

	//! For the currently select object add the extraString info
	//! in a format that matches the Nautical Almanac.
	void extraInfo(StelCore* core, const StelObjectP& selectedObject);

	//! Used to display the extraInfoStrings in standard "paired" lines (for example gha/dev)
	void displayStandardInfo(const StelObjectP& selectedObject, NavStarsCalculator& calc, const QString& extraText);

	//! Used to display the extraInfoStrings in tabulated form more suited to students of CN
	//! as found when using Nautical Almanacs.
	void displayTabulatedInfo(const StelObjectP& selectedObject, NavStarsCalculator& calc, const QString& extraText);

	//! Given two QStrings return in a format consistent with the
	//! property setting of "withTables".
	//! @param QString a The cell left value
	//! @param QString b The cell right value
	//! @return QString The representation of the extraString info.
	QString oneRowTwoCells(const QString& a, const QString& b, const QString& extra, bool tabulatedView);

	bool isPermittedObject(const QString& s);

private slots:
	//! Call when button "Save settings" in main GUI are pressed
	void saveSettings() { saveConfiguration(); }
	void setUseDecimalDegrees(bool flag);

signals:
	//! Emitted when display of markers have been changed.
	void navStarsMarksChanged(bool b);
	void enableAtStartupChanged(bool b);
	void highlightWhenVisibleChanged(bool b);
	void limitInfoToNavStarsChanged(bool b);
	void upperLimbChanged(bool b);
	void tabulatedDisplayChanged(bool b);
	void showExtraDecimalsChanged(bool b);
	void flagUseUTCTimeChanged(bool b);

private:
	NavStarsWindow* mainWindow;
	StelPropertyMgr* propMgr;
	QSettings* conf;	

	// The current set of navigational stars
	NavigationalStarsSet currentNSSet;

	bool enableAtStartup;
	bool starLabelsState;
	bool upperLimb;
	bool highlightWhenVisible;
	bool limitInfoToNavStars;
	bool tabulatedDisplay;
	bool useUTCTime;

	QString timeZone;
	QVector<QString> permittedObjects;

	//! List of the navigational stars' HIP numbers.
	QList<int> starNumbers;
	//! List of pointers to the objects representing the stars.
	QVector<StelObjectP> stars;
	
	StelTextureSP markerTexture;
	//! Color used to paint each star's marker and additional label.
	Vec3f markerColor;	
	LinearFader markerFader;	

	//! Button for the bottom toolbar.
	StelButton* toolbarButton;
};


#include <QObject>
#include "StelPluginInterface.hpp"

//! This class is used by Qt to manage a plug-in interface
class NavStarsStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID StelPluginInterface_iid)
	Q_INTERFACES(StelPluginInterface)
public:
	virtual StelModule* getStelModule() const;
	virtual StelPluginInfo getPluginInfo() const;
	virtual QObjectList getExtensionList() const { return QObjectList(); }
};

#endif // NAVSTARS_HPP
