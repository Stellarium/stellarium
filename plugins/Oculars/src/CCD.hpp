/*
 * Copyright (C) 2010 Timothy Reaves
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

#ifndef CCD_HPP
#define CCD_HPP

#include <QObject>
#include <QString>
#include <QSettings>

class Telescope;
class Lens;

//! @ingroup oculars
//! @class CCD describes CCD properties for the Oculars plugin.
//! pixel count, sensor size and pixel size are obviously related, and should all be given in data sheets or manuals.
//! Sometimes one of them is lacking, or given only approximately like "APS-C" which can have some tolerance.
//! As a consequence, it seems natural to assume one fixed and one updateable when setting one of those.
//! The logic used here:
//! * setResolutionX|Y: set pixel count in X|Y, and compute  pixelHeight|pixelWidth = chipWidth|chipHeight / resolutionX|Y
//! * setChipWidth|setChipHeight: set chip width|height, and compute  pixelHeight|pixelWidth = chipWidth|chipHeight / resolutionX|Y
//! * setPixelWidth|setPixelHeight: set pixel width|height, and compute chipWidth|chipHeight=pixelWidth|pixelHeight * resolutionX|Y
class CCD : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QString name         READ name          WRITE setName)
	Q_PROPERTY(int resolutionX      READ resolutionX   WRITE setResolutionX)
	Q_PROPERTY(int resolutionY      READ resolutionY   WRITE setResolutionY)
	Q_PROPERTY(double chipWidth     READ chipWidth     WRITE setChipWidth)
	Q_PROPERTY(double chipHeight    READ chipHeight    WRITE setChipHeight)
	Q_PROPERTY(double chipRotAngle  READ chipRotAngle  WRITE setChipRotAngle)
	Q_PROPERTY(int binningX         READ binningX      WRITE setBinningX)
	Q_PROPERTY(int binningY         READ binningY      WRITE setBinningY)
	Q_PROPERTY(double hasOAG        READ hasOAG        WRITE setHasOAG)
	Q_PROPERTY(double prismHeight   READ prismHeight   WRITE setPrismHeight)
	Q_PROPERTY(double prismWidth    READ prismWidth    WRITE setPrismWidth)
	Q_PROPERTY(double prismDistance READ prismDistance WRITE setPrismDistance)
	Q_PROPERTY(double prismPosAngle READ prismPosAngle WRITE setPrismPosAngle)

public:
	CCD();
	Q_INVOKABLE CCD(const QObject& other);
	virtual ~CCD() Q_DECL_OVERRIDE;
	static CCD* ccdFromSettings(QSettings* settings, int ccdIndex);
	void writeToSettings(QSettings * settings, const int index);
	static CCD* ccdModel();

	QString name() const;
	void setName(QString name);
	int getCCDID();
	int resolutionX() const;
	void setResolutionX(int resolution);
	int resolutionY() const;
	void setResolutionY(int resolution);
	double chipWidth() const;
	void setChipWidth(double width);
	double chipHeight() const;
	void setChipHeight(double height);
	double chipRotAngle() const;
	void setChipRotAngle(double angle);
	int binningX() const;
	void setBinningX(int binning);
	int binningY() const;
	void setBinningY(int binning);
	bool hasOAG() const;
	void setHasOAG(bool oag);
	double prismDistance() const;
	void setPrismDistance(double distance);
	double prismHeight() const;
	void setPrismHeight(double height);
	double prismWidth() const;
	void setPrismWidth(double width);
	double prismPosAngle() const;
	void setPrismPosAngle(double angle);	

	/**
	  * The formula for this calculation comes from the Yerkes observatory.
	  * fov degrees = 2PI/360degrees * chipDimension mm / telescope FL mm
	  */
	double getActualFOVx(const Telescope *telescope, const Lens *lens) const;
	double getActualFOVy(const Telescope *telescope, const Lens *lens) const;
	//! Horizontal angular resolution in degrees per pixel in the middle of the field of view
	double getCentralAngularResolutionX(const Telescope *telescope, const Lens *lens) const;
	//! Vertical angular resolution in degrees per pixel in the middle of the field of view
	double getCentralAngularResolutionY(const Telescope *telescope, const Lens *lens) const;
	//! focuser size in inches
	double getFocuserFOV(const Telescope *telescope, const Lens *lens, double focuserSize) const;
	double getInnerOAGRadius(const Telescope *telescope, const Lens *lens) const;
	double getOuterOAGRadius(const Telescope *telescope, const Lens *lens) const;
	double getOAGActualFOVx(const Telescope *telescope, const Lens *lens) const;
	QMap<int, QString> propertyMap();
private:
	QString m_name;
	//! total resolution width in pixels
	int m_resolutionX;
	//! total resolution height in pixels
	int m_resolutionY;
	//! chip width in millimeters
	double m_chipWidth;
	//! chip height in millimeters
	double m_chipHeight;
	//! chip rotation angle around its axis (degrees)
	double m_chipRotAngle;
	//! Binning for axes X
	int m_binningX;
	//! Binning for axes Y
	int m_binningY;
	//! Show off axis guider view
	bool m_has_oag;
	//! OAG prism height (millimeters)
	double m_oag_prismHeight;
	//! OAG prism width (millimeters)
	double m_oag_prismWidth;
	//! OAG prism distance from the axis center (millimeters)
	double m_oag_prismDistance;
	//! OAG prism position angle (degrees)
	double m_oag_prismPosAngle;
	static const QMap<int, QString> mapping;
};


#endif /* CCD_HPP */
