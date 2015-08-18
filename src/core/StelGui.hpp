/*
 * Stellarium
 * Copyright (C) 2015 Guillaume Chereau
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

#include <QObject>
#include <QVariantList>

// This object is only used as a proxy to provide the list of registered
// buttons to the QML gui.
class StelGui : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QVariantList buttons MEMBER buttons NOTIFY changed)
	Q_PROPERTY(bool autoHideHorizontalButtonBar MEMBER autoHideHorizontalButtonBar NOTIFY changed)
	Q_PROPERTY(bool autoHideVerticalButtonBar MEMBER autoHideVerticalButtonBar NOTIFY changed)
	Q_PROPERTY(bool helpDialogVisible MEMBER helpDialogVisible NOTIFY changed)
	Q_PROPERTY(bool configurationDialogVisible MEMBER configurationDialogVisible NOTIFY changed)
	Q_PROPERTY(bool searchDialogVisible MEMBER searchDialogVisible NOTIFY changed)
	Q_PROPERTY(bool viewDialogVisible MEMBER viewDialogVisible NOTIFY changed)
	Q_PROPERTY(bool dateTimeDialogVisible MEMBER dateTimeDialogVisible NOTIFY changed)
	Q_PROPERTY(bool locationDialogVisible MEMBER locationDialogVisible NOTIFY changed)
	Q_PROPERTY(bool shortcutsDialogVisible MEMBER shortcutsDialogVisible NOTIFY changed)

	Q_PROPERTY(QString locationName READ getLocationName NOTIFY changed)
	Q_PROPERTY(QStringList languages READ getLanguages NOTIFY changed)
	Q_PROPERTY(QString language READ getLanguage WRITE setLanguage NOTIFY changed)
	Q_PROPERTY(QString skyLanguage READ getSkyLanguage WRITE setSkyLanguage NOTIFY changed)
	Q_PROPERTY(int infoTextFilters MEMBER infoTextFilters NOTIFY changed)
	Q_PROPERTY(QString selectionInfoText MEMBER selectionInfoText NOTIFY updated)

signals:
	void changed();
	void updated();

public:

	// This is copied from StelObject.hpp, I need to put it into a QObject
	// so that it can be binded to qml.
	// TODO: find a better way.
	enum InfoStringGroupFlags
	{
		Name			= 0x00000001, //!< An object's name
		CatalogNumber		= 0x00000002, //!< Catalog numbers
		Magnitude		= 0x00000004, //!< Magnitude related data
		RaDecJ2000		= 0x00000008, //!< The equatorial position (J2000 ref)
		RaDecOfDate		= 0x00000010, //!< The equatorial position (of date)
		AltAzi			= 0x00000020, //!< The position (Altitude/Azimuth)
		Distance		= 0x00000040, //!< Info about an object's distance
		Size			= 0x00000080, //!< Info about an object's size
		Extra			= 0x00000100, //!< Derived class-specific extra fields
		HourAngle		= 0x00000200, //!< The hour angle + DE (of date)
		AbsoluteMagnitude	= 0x00000400, //!< The absolute magnitude
		GalacticCoord		= 0x00000800, //!< The galactic position
		ObjectType		= 0x00001000, //!< The type of the object (star, planet, etc.)
		EclipticCoord		= 0x00002000, //!< The ecliptic position
		EclipticCoordXYZ	= 0x00004000, //!< The ecliptic position, XYZ of VSOP87A (used mainly for debugging, not public)
		PlainText		= 0x00010000,  //!< Strip HTML tags from output
		AllInfo			= Name|CatalogNumber|Magnitude|RaDecJ2000|RaDecOfDate|AltAzi|Distance|Size|Extra|HourAngle|
									   AbsoluteMagnitude|GalacticCoord|ObjectType|EclipticCoord|EclipticCoordXYZ,
		ShortInfo		= Name|CatalogNumber|Magnitude|RaDecJ2000,
	};
	Q_ENUM(InfoStringGroupFlags)

	StelGui();
	void addButton(QString pixOn, QString pixOff,
				   QString action, QString groupName,
				   QString beforeActionName = QString());
	QString getLocationName() const;

	QStringList getLanguages() const;
	QString getLanguage() const;
	void setLanguage(QString v);
	QString getSkyLanguage() const;
	void setSkyLanguage(QString v);

	int getInfoStringGroupFlags() const;
	void setInfoStringGroupFlags(int v);

	void update();

	Q_INVOKABLE QVariantList jdToDate(double jd) const;
	Q_INVOKABLE double jdFromDate(int Y, int M, int D, int h, int m, int s) const;

private slots:
	void quit();

private:
	QVariantList buttons;
	bool autoHideHorizontalButtonBar;
	bool autoHideVerticalButtonBar;

	bool helpDialogVisible;
	bool configurationDialogVisible;
	bool searchDialogVisible;
	bool viewDialogVisible;
	bool dateTimeDialogVisible;
	bool locationDialogVisible;
	bool shortcutsDialogVisible;

	int infoTextFilters;
	QString selectionInfoText;
};
