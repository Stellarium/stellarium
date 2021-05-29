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

#pragma once

#include "OcularsConfig.hpp"

#include <QExplicitlySharedDataPointer>
#include <QObject>
#include <QSettings>
#include <QString>
#include <QtGlobal>

class Lens;
class Telescope;

//! @ingroup oculars
class CCD : public QObject
{
	Q_OBJECT
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
	Q_DISABLE_COPY_MOVE(CCD)
#endif
	Q_PROPERTY(QString name READ name WRITE setName)
	Q_PROPERTY(int resolutionX READ resolutionX WRITE setResolutionX)
	Q_PROPERTY(int resolutionY READ resolutionY WRITE setResolutionY)
	Q_PROPERTY(double chipWidth READ chipWidth WRITE setChipWidth)
	Q_PROPERTY(double chipHeight READ chipHeight WRITE setChipHeight)
	Q_PROPERTY(double rotationAngle READ rotationAngle WRITE setRotationAngle)
	Q_PROPERTY(int binningX READ binningX WRITE setBinningX)
	Q_PROPERTY(int binningY READ binningY WRITE setBinningY)
	Q_PROPERTY(double hasOAG READ hasOAG WRITE setHasOAG)
	Q_PROPERTY(double prismHeight READ prismHeight WRITE setPrismHeight)
	Q_PROPERTY(double prismWidth READ prismWidth WRITE setPrismWidth)
	Q_PROPERTY(double prismDistance READ prismDistance WRITE setPrismDistance)
	Q_PROPERTY(double prismPosAngle READ prismPosAngle WRITE setPrismPosAngle)

public:
	/// Creates a new instance of CCD.
	//! The newly created instance will have values initialized to represent a usable CCD model.
	Q_INVOKABLE explicit CCD(QObject * parent = nullptr);
	~CCD() override = default;

	void        initFromSettings(QSettings * settings, int ccdIndex);
	void        writeToSettings(QSettings * settings, int index) const;

	auto        name() const -> QString;
	void        setName(QString name);
	auto        resolutionX() const -> int;
	void        setResolutionX(int resolution);
	auto        resolutionY() const -> int;
	void        setResolutionY(int resolution);
	auto        chipWidth() const -> double;
	void        setChipWidth(double width);
	auto        chipHeight() const -> double;
	void        setChipHeight(double height);
	auto        rotationAngle() const -> double;
	void        setRotationAngle(double angle);
	auto        binningX() const -> int;
	void        setBinningX(int binning);
	auto        binningY() const -> int;
	void        setBinningY(int binning);
	auto        hasOAG() const -> bool;
	void        setHasOAG(bool oag);
	auto        prismDistance() const -> double;
	void        setPrismDistance(double distance);
	auto        prismHeight() const -> double;
	void        setPrismHeight(double height);
	auto        prismWidth() const -> double;
	void        setPrismWidth(double width);
	auto        prismPosAngle() const -> double;
	void        setPrismPosAngle(double angle);

	/**
    * The formula for this calculation comes from the Yerkes observatory.
    * fov degrees = 2PI/360degrees * chipDimension mm / telescope FL mm
    */
	auto        getActualFOVx(Telescope * telescope, Lens * lens) const -> double;
	auto        getActualFOVy(Telescope * telescope, Lens * lens) const -> double;
	//! focuser size in inches
	static auto getFocuserFOV(Telescope * telescope, Lens * lens, double focuserSize) -> double;
	auto        getInnerOAGRadius(Telescope * telescope, Lens * lens) const -> double;
	auto        getOuterOAGRadius(Telescope * telescope, Lens * lens) const -> double;
	auto        getOAGActualFOVx(Telescope * telescope, Lens * lens) const -> double;
	static auto propertyMap() -> QMap<int, QString>;

private:
	int     m_binningX{ 1 };
	int     m_binningY{ 1 };
	double  m_chipWidth{ CCDDefaultChipWidth };
	double  m_chipHeight{ CCDDefaultChipHeigth };
	QString m_name{ "New CCD" };
	bool    m_oagEquipped{ false };
	double  m_oagPrismHeight{ 0.0 };
	double  m_oagPrismWidth{ 0.0 };
	double  m_oagPrismDistance{ 0.0 };
	double  m_oagPrismPositionAngle{ 0.0 }; // degrees
	int     m_resolutionX{ CCDDefaultResolutionX };
	int     m_resolutionY{ CCDDefaultResolutionY };
	double  m_rotationAngle{ 0.0 }; // degrees
};

auto operator<<(QDebug debug, const CCD & ccd) -> QDebug;
auto operator<<(QDebug debug, const CCD * ccd) -> QDebug;

Q_DECLARE_METATYPE(CCD *);
