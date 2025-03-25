/*
 * Stellarium
 * Copyright (C) 2006 Fabien Chereau
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

#ifndef STELSKYCULTUREMGR_HPP
#define STELSKYCULTUREMGR_HPP

#include <QObject>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>
#include "StelModule.hpp"

class StelTranslator;

//! @class StelSkyCulture
//! Store basic info about a sky culture for Stellarium.
//! Different human cultures have used different names for stars, and visualised
//! different constellations in the sky (and in different parts of the sky).
class StelSkyCulture
{
	Q_GADGET
public:
	//! Possible values for boundary lines between constellations.
	//! Most traditional skies do not have boundaries.
	//! Some atlases in the 18th and 19th centuries had curved boundaries that differed between authors.
	//! Only IAU implemented "approved" boundaries, but a similar technique could be used for other skycultures.
	enum class BoundariesType
	{
		None = -1,
		IAU,
		Own
	};
	Q_ENUM(BoundariesType)

	//! Since V0.19. A rough classification scheme that may allow filtering and at least some rough idea of quality control.
	//! In future versions, this scheme could be refined or changed, and external reviewers
	//! can probably provide more quality control. For now, we can at least highlight and discern
	//! "nice tries" from scientific work.
	//! The classes are not "quality grades" in ascending order, but also allow estimates about particular content.
	//! INCOMPLETE requires improvements, and PERSONAL usually means "nice, but not even Stellarium developers believe in it".
	enum CLASSIFICATION
	{
		INCOMPLETE=0,	//! Looks like there is something interesting to it, but lacks references. Should evolve into one of the other kinds.
				//! There are some examples in our repositories from previous times that should be improved,
				//! else no new ones should be accepted.
		PERSONAL,	//! Privately developed after ca. 1950, not based on published ethnographic or historical research, not supported by a noteworthy community.
		TRADITIONAL,	//! Most "living" skycultures. May have evolved over centuries, with mixed influences from other cultures.
				//! Also for self-presentations by members of respective cultures, indigenous peoples or tribes.
				//! Description should provide a short description of the people and traditions, and the "cosmovision" of the people,
				//! some celestial myths, background information about the constellations (e.g. what does a "rabbit ghost" or "yellow man" mean for you?)
				//! Star names with meaning should be translated to English.
				//! Please provide "further reading" links for more information.
		ETHNOGRAPHIC,	//! Created by foreigners doing ethnographic fieldwork in modern times.
				//! This usually is an "outside view", e.g. from ethnographic fieldwork, missionary reports, travelers, "adventurers" of the 19th century etc.
				//! Description should provide a short description of the way this skyculture has been recorded, about the
				//! people and traditions, and the "cosmovision" of the people, some celestial myths,
				//! background information about the constellations (e.g. what does a "rabbit ghost" or "yellow man" mean for them?)
				//! Star names with meaning should be translated to English.
				//! This is often published in singular rare to find books, or found in university collections or museum archives.
				//! These should come with links for published books, or how to find this information elsewhere.
		HISTORICAL,	//! Skyculture from past time, recreated from textual transmission by historians.
				//! Typically nobody alive today shares the world view of these past cultures.
				//! The description should provide some insight over sources and how data were retrieved and interpreted,
				//! and should provide references to (optimally: peer-reviewed) published work.
		SINGLE,		//! Implementation of a single book or atlas usually providing a "snapshot" of a traditional skyculture.
				//! e.g. Bayer, Schiller, Hevelius, Bode, Rey, ...
				//! Content (star names, artwork, spelling, ...) should not deviate from what the atlas contains.
				//! The description should provide information about the presented work, and if possible a link to a digital online version.
		COMPARATIVE	//! Special-purpose compositions of artwork from one and stick figures from another skyculture. These figures
				//! sometimes will appear not to fit together well. This may be intended, to explain and highlight just those differences!
				//! The description text must clearly explain and identify both sources and how these differences should be interpreted.
	};
	Q_ENUM(CLASSIFICATION)

	//! Sky culture identifier (usually same as directory name)
	QString id;
	//! Full directory path
	QString path;
	//! English name
	QString englishName;
	//! The license
	QString license;
	//! The name of region following the United Nations geoscheme UN~M49 https://unstats.un.org/unsd/methodology/m49/
	//! For skycultures of worldwide applicability (mostly those adhering to IAU constellation borders), use "World".
	QString region;
	//! Type of the boundaries
	BoundariesType boundariesType;
	//! JSON data describing the constellations (names, lines, artwork)
	QJsonArray constellations;
	//! JSON data describing boundaries of the constellations
	QJsonArray boundaries;
	//! Epoch for boundary definition. E.g. "J2000" (default), "B1875", "J2050.0", "B1950.5", "JD1234567.89" or just any JD value
	//! The first two examples are treated most efficiently.
	QString boundariesEpoch;
	//! JSON data describing asterism lines and names
	QJsonArray asterisms;
	//! Classification of sky culture (enum)
	CLASSIFICATION classification;
	//! JSON data containing culture-specific names of celestial objects (stars, planets, DSO)
	QJsonObject names;
	//! JSON data describing the policy on the usage of native names vs the English ones
	QJsonArray langsUseNativeNames;
	//! Whether to show common names in addition to the culture-specific ones
	bool fallbackToInternationalNames = false;
};

//! @class StelSkyCultureMgr
//! Manage sky cultures for stellarium.
//! In the installation data directory and user data directory are the "skycultures"
//! sub-directories containing one sub-directory per sky culture.
//! This sub-directory name is that we refer to as sky culture ID here.
//! @author Fabien Chereau
class StelSkyCultureMgr : public StelModule
{
	Q_OBJECT
	Q_PROPERTY(QString currentSkyCultureID
		   READ getCurrentSkyCultureID
		   WRITE setCurrentSkyCultureID
		   NOTIFY currentSkyCultureIDChanged)
	Q_PROPERTY(QString defaultSkyCultureID
		   READ getDefaultSkyCultureID
		   WRITE setDefaultSkyCultureID
		   NOTIFY defaultSkyCultureIDChanged)
	Q_PROPERTY(StelObject::CulturalDisplayStyle screenLabelStyle
		   READ getScreenLabelStyle
		   WRITE setScreenLabelStyle
		   NOTIFY screenLabelStyleChanged)
	Q_PROPERTY(StelObject::CulturalDisplayStyle infoLabelStyle
		   READ getInfoLabelStyle
		   WRITE setInfoLabelStyle
		   NOTIFY infoLabelStyleChanged)

public:
	StelSkyCultureMgr();
	~StelSkyCultureMgr();
	
	//! Initialize the StelSkyCultureMgr object.
	//! Gets the default sky culture name from the application's settings,
	//! sets that sky culture by calling setCurrentSkyCultureID().
	void init();
	
public slots:
	//! Get the current sky culture English name.
	QString getCurrentSkyCultureEnglishName() const;
	//! Get the current sky culture translated name.
	QString getCurrentSkyCultureNameI18() const;
	//! Set the sky culture from i18n name.
	//! @return true on success; false and doesn't change if skyculture is invalid.
	bool setCurrentSkyCultureNameI18(const QString& cultureName);
	
	//! Get the current sky culture ID.
	QString getCurrentSkyCultureID() const {return currentSkyCulture.id;}
	//! Set the current sky culture from the ID.
	//! @param id the sky culture ID.
	//! @return true on success; else false.
	bool setCurrentSkyCultureID(const QString& id);
	//! Reload the current sky culture
	void reloadSkyCulture();

	//! Get the type of boundaries of the current sky culture
	//! Config option: info/boundaries
	StelSkyCulture::BoundariesType getCurrentSkyCultureBoundariesType() const;

	//! Get the classification index for the current sky culture
	//! Config option: info/classification
	//! Possible values:
	//! - scientific (1)
	//! - traditional (2; using by default)
	//! - personal (3)
	//! - single (4)
	int getCurrentSkyCultureClassificationIdx() const;

	//! @return a localized HTML description of the classification for the current sky culture
	QString getCurrentSkyCultureHtmlClassification() const;

	//! @return a localized HTML description of the license given in markdown
	QString getCurrentSkyCultureHtmlLicense() const;

	//! @return a localized HTML description of the region for the current sky culture
	QString getCurrentSkyCultureHtmlRegion() const;

	//! Returns a localized HTML description for the current sky culture.
	//! @return a HTML description of the current sky culture, suitable for display
	QString getCurrentSkyCultureHtmlDescription();
	
	//! Get the default sky culture ID
	QString getDefaultSkyCultureID() {return defaultSkyCultureID;}
	//! Set the default sky culture from the ID.
	//! @param id the sky culture ID.
	//! @return true on success; else false.
	bool setDefaultSkyCultureID(const QString& id);
	
	//! Get a list of sky culture names in English.
	//! @return A new-line delimited list of English sky culture names.
	QString getSkyCultureListEnglish(void) const;
	
	//! Get a list of sky culture names in the current language.
	//! @return A list of translated sky culture names.
	QStringList getSkyCultureListI18(void) const;

	//! Get a list of sky culture IDs
	QStringList getSkyCultureListIDs(void) const;

	//! Returns a map from sky culture IDs/folders to sky culture names.
	QMap<QString, StelSkyCulture> getDirToNameMap() const { return dirToNameEnglish; }

	//! Returns the screen labeling setting for the currently active skyculture
	StelObject::CulturalDisplayStyle getScreenLabelStyle();
	//! Sets the screen labeling setting for the currently active skyculture
	void setScreenLabelStyle(StelObject::CulturalDisplayStyle &style);

	//! Returns the InfoString labeling setting for the currently active skyculture
	StelObject::CulturalDisplayStyle getInfoLabelStyle();
	//! sets the InfoString labeling setting for the currently active skyculture
	void setInfoLabelStyle(StelObject::CulturalDisplayStyle &style);

signals:
	//! Emitted whenever the default sky culture changed.
	//! @see setDefaultSkyCultureID
	void defaultSkyCultureIDChanged(const QString& id);

	//! Emitted when the current sky culture changes
	void currentSkyCultureIDChanged(const QString& id);

	//! Emitted when the current sky culture changes
	void currentSkyCultureChanged(const StelSkyCulture& culture);

	//! Emitted when InfoLabelStyle has changed
	void infoLabelStyleChanged(StelObject::CulturalDisplayStyle &style);
	//! Emitted when ScreenLabelStyle has changed
	void screenLabelStyleChanged(StelObject::CulturalDisplayStyle &style);

private:
	//! Scan all sky cultures to get their names and other properties.
	void makeCulturesList();

	//! Read the English name of the sky culture from description file.
	//! @param idFromJSON the id from \p index.json that will be used as a default name if an error occurs.
	QString getSkyCultureEnglishName(const QString& idFromJSON) const;
	//! Get the culture name in English associated with a specified directory.
	//! @param directory The directory name.
	//! @return The English name for the culture associated with directory.
	QString directoryToSkyCultureEnglish(const QString& directory) const;
	
	//! Get the culture name translated to current language associated with 
	//! a specified directory.
	//! @param directory The directory name.
	//! @return The translated name for the culture associated with directory.
	QString directoryToSkyCultureI18(const QString& directory) const;
	
	//! Get the directory associated with a specified translated culture name.
	//! @param cultureName The culture name in the current language.
	//! @return The directory associated with cultureName.
	QString skyCultureI18ToDirectory(const QString& cultureName) const;

	QString descriptionMarkdownToHTML(const QString& markdown, const QString& descrPath);
	QString convertMarkdownLevel2Section(const QString& markdown, const QString& sectionName,
	                                     qsizetype bodyStartPos, qsizetype bodyEndPos, const StelTranslator& trans);
	std::pair<QString/*color*/,QString/*info*/> getLicenseDescription(const QString& license, const bool singleLicenseForAll) const;

	QMap<QString, StelSkyCulture> dirToNameEnglish;
	
	StelSkyCulture currentSkyCulture;
	
	QString defaultSkyCultureID;
};

#endif // STELSKYCULTUREMGR_HPP
