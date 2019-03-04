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

//! @class StelSkyCulture
//! Store basic info about a sky culture for stellarium.
class StelSkyCulture
{
public:
	//! English name
	QString englishName;
	//! Name of the author
	QString author;
	//! The license
	QString license;
	//! Type of the boundaries (-1=none;0=generic;1=own)
	int boundariesIdx;
	//! Classification of sky culture (1=scientific;2=traditional;3=single;4=personal)
	int classificationIdx;
};

//! @class StelSkyCultureMgr
//! Manage sky cultures for stellarium.
//! Different human cultures have used different names for stars, and visualised
//! different constellations in the sky (and in different parts of the sky).
//! In the installation data directory and user data directory are the "skycultures"
//! sub-directories containing one sub-directory per sky culture.
//! This sub-directory name is that we refer to as sky culture ID here.
//! @author Fabien Chereau
class StelSkyCultureMgr : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QString currentSkyCultureID
		   READ getCurrentSkyCultureID
		   WRITE setCurrentSkyCultureID
		   NOTIFY currentSkyCultureChanged)
	Q_PROPERTY(QString defaultSkyCultureID
		   READ getDefaultSkyCultureID
		   WRITE setDefaultSkyCultureID
		   NOTIFY defaultSkyCultureChanged)

public:
	StelSkyCultureMgr();
	~StelSkyCultureMgr();
	
	//! Initialize the StelSkyCultureMgr object.
	//! Gets the default sky culture name from the application's settings,
	//! sets that sky culture by calling setCurrentSkyCultureID().
	void init();
	
	//! Get the current sky culture.
	StelSkyCulture getSkyCulture() const {return currentSkyCulture;}
	
public slots:
	//! Get the current sky culture English name.
	QString getCurrentSkyCultureEnglishName() const;
	//! Get the current sky culture translated name.
	QString getCurrentSkyCultureNameI18() const;
	//! Set the sky culture from i18n name.
	//! @return true on success; false and doesn't change if skyculture is invalid.
	bool setCurrentSkyCultureNameI18(const QString& cultureName);
	
	//! Get the current sky culture ID.
	QString getCurrentSkyCultureID() const {return currentSkyCultureDir;}
	//! Set the current sky culture from the ID.
	//! @param id the sky culture ID.
	//! @return true on success; else false.
	bool setCurrentSkyCultureID(const QString& id);

	//! Get the type of boundaries of the current sky culture
	//! Config option: info/boundaries
	//! Keys:
	//! - none (-1; using by default)
	//! - generic (0)
	//! - own (1)
	int getCurrentSkyCultureBoundariesIdx() const;

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

	//! Returns a localized HTML description for the current sky culture.
	//! @return a HTML description of the current sky culture, suitable for display
	QString getCurrentSkyCultureHtmlDescription() const;
	
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

signals:
	//! Emitted whenever the default sky culture changed.
	//! @see setDefaultSkyCultureID
	void defaultSkyCultureChanged(const QString& id);

	//! Emitted when the current sky culture changes
	void currentSkyCultureChanged(const QString& id);
	
private:
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
	
	QMap<QString, StelSkyCulture> dirToNameEnglish;
	
	// The directory containing data for the culture used for constellations, etc.. 
	QString currentSkyCultureDir;
	StelSkyCulture currentSkyCulture;
	
	QString defaultSkyCultureID;
};

#endif // STELSKYCULTUREMGR_HPP
