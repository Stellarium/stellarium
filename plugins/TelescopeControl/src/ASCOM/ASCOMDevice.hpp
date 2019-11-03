/*
 * Copyright (C) 2019 Gion Kunz <gion.kunz@gmail.com>
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

#ifndef ASCOMDEVICE_HPP
#define ASCOMDEVICE_HPP

#include <QObject>
#include <QStringList>
#include "../common/OLE.hpp"

class ASCOMDevice final : public QObject
{
	Q_OBJECT

public:
	struct ASCOMCoordinates
	{
		double RA = 0.0;
		double DEC = 0.0;
	};

	enum ASCOMEquatorialCoordinateType
	{
		Other, Topocentric, J2000, J2050, B1950
	};

	static QString showDeviceChooser(QString previousDeviceId = "");

	ASCOMDevice(QObject* parent = Q_NULLPTR, QString ascomDeviceId = Q_NULLPTR);
	
	bool isDeviceConnected() const;
	bool isParked() const;
	bool connect();
	bool disconnect();
	ASCOMCoordinates position() const;
	void slewToCoordinates(ASCOMCoordinates coords);
	void syncToCoordinates(ASCOMCoordinates coords);
	void abortSlew();
	ASCOMEquatorialCoordinateType getEquatorialCoordinateType();
	bool doesRefraction();
	ASCOMCoordinates getCoordinates();

signals:
	void deviceConnected();
	void deviceDisconnected();

private:
	IDispatch* pTelescopeDispatch;
	bool mConnected = false;
	ASCOMCoordinates mCoordinates;
	QString mAscomDeviceId;
	static const wchar_t* LSlewToCoordinatesAsync;
	static const wchar_t* LSyncToCoordinates;
	static const wchar_t* LAbortSlew;
	static const wchar_t* LConnected;
	static const wchar_t* LAtPark;
	static const wchar_t* LEquatorialSystem;
	static const wchar_t* LDoesRefraction;
	static const wchar_t* LRightAscension;
	static const wchar_t* LDeclination;
	static const wchar_t* LChoose;
};

#endif // ASCOMDEVICE_HPP
