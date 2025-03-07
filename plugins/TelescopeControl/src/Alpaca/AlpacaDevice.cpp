/*
 * Copyright (C) 2019 Gion Kunz <gion.kunz@gmail.com> (ASCOM)
 * Copyright (C) 2025 Georg Zotti (Alpaca port)
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

#include <QDebug>

#include "AlpacaDevice.hpp"
//#include <comdef.h>

AlpacaDevice::AlpacaDevice(QObject* parent, QString AlpacaDeviceId) : QObject(parent),
	alpacaDeviceNumber(AlpacaDeviceId.toLong())
{}

bool AlpacaDevice::connect()
{
	/*
	if (mConnected) return true;

	BOOL initResult = OleInit(COINIT_APARTMENTTHREADED);
	HRESULT hResult = OleCreateInstance(reinterpret_cast<const wchar_t*>(mAlpacaDeviceId.toStdWString().c_str()), &pTelescopeDispatch);

	if (!initResult || FAILED(hResult))
	{
		qWarning() << "Initialization failed for device: " << mAlpacaDeviceId;
		return false;
	}

	VARIANT v1 = OleBoolToVariant(TRUE);

	hResult = OlePropertyPut(pTelescopeDispatch, nullptr, const_cast<wchar_t*>(LConnected), 1, v1);

	if (FAILED(hResult))
	{
		qWarning() << "Could not connect to device: " << mAlpacaDeviceId;
		return false;
	}
	
	mConnected = true;
	return true;
	*/
	// TODO: PUT call?
	qWarning() << "AlpacaDevice::connect() not yet implemented";
	return true;
}

bool AlpacaDevice::disconnect()
{
	/*
	if (!mConnected) return true;

	VARIANT v1 = OleBoolToVariant(FALSE);
	HRESULT hResult = OlePropertyPut(pTelescopeDispatch, nullptr, const_cast<wchar_t*>(LConnected), 1, v1);

	if (FAILED(hResult))
	{
		qWarning() << "Could not disconnect device: " << mAlpacaDeviceId;
		return false;
	}
	
	pTelescopeDispatch->Release();

	mConnected = false;
	return true;
	*/
	// TODO: PUT call?
	qWarning() << "AlpacaDevice::disconnect() not yet implemented";
	return true;
}

AlpacaDevice::AlpacaCoordinates AlpacaDevice::position() const
{
	return mCoordinates;
}


void AlpacaDevice::slewToCoordinates(AlpacaDevice::AlpacaCoordinates coords) 
{
	/*
	if (!mConnected) return;

	VARIANT v1 = OleDoubleToVariant(coords.RA);
	VARIANT v2 = OleDoubleToVariant(coords.DEC);

	HRESULT hResult = OleMethodCall(pTelescopeDispatch, nullptr, const_cast<wchar_t*>(LSlewToCoordinatesAsync), 2, v1, v2);
	if (FAILED(hResult))
	{
		qDebug() << "Slew failed for device: " << mAlpacaDeviceId;
	}
	*/
	// TODO: PUT call
	qWarning() << "AlpacaDevice: slewToCoordinates() not yet implemented";
}

void AlpacaDevice::syncToCoordinates(AlpacaCoordinates coords)
{
	/*
	if (!mConnected) return;

	VARIANT v1 = OleDoubleToVariant(coords.RA);
	VARIANT v2 = OleDoubleToVariant(coords.DEC);

	HRESULT hResult = OleMethodCall(pTelescopeDispatch, nullptr, const_cast<wchar_t*>(LSyncToCoordinates), 2, v1, v2);
	if (FAILED(hResult))
	{
		qDebug() << "Could not sync to coordinates for device: " << mAlpacaDeviceId;
	}
	*/
	// TODO: PUT call
	qWarning() << "AlpacaDevice: syncToCoordinates() not yet implemented";
}

void AlpacaDevice::abortSlew()
{
	/*
	if (!mConnected) return;

	HRESULT hResult = OleMethodCall(pTelescopeDispatch, nullptr, const_cast<wchar_t*>(LAbortSlew));
	if (FAILED(hResult))
	{
		qCritical() << "Could not abort slew for device: " << mAlpacaDeviceId;
	}
	*/
	// TODO: PUT call
	qWarning() << "AlpacaDevice: abortSlew() not yet implemented";
}

bool AlpacaDevice::isDeviceConnected() const
{
	/*
	if (!mConnected) return false;

	VARIANT v1;
	HRESULT hResult = OlePropertyGet(pTelescopeDispatch, &v1, const_cast<wchar_t*>(LConnected));

	if (FAILED(hResult))
	{
		qDebug() << "Could not get connected state for device: " << mAlpacaDeviceId;
		return false;
	}

	return v1.boolVal == -1;
	*/
	// TODO: If it makes sense at all, GET call
	return true;
}

bool AlpacaDevice::isParked() const
{
	/*
	if (!mConnected) return true;

	VARIANT v1;
	HRESULT hResult = OlePropertyGet(pTelescopeDispatch, &v1, const_cast<wchar_t*>(LAtPark));

	if (FAILED(hResult))
	{
		qDebug() << "Could not get AtPark state for device: " << mAlpacaDeviceId;
		return false;
	}

	return v1.boolVal == -1;
	*/
	// TODO: GET call
	return false;
}


AlpacaDevice::AlpacaEquatorialCoordinateType AlpacaDevice::getEquatorialCoordinateType()
{
	/*
	if (!mConnected) return AlpacaDevice::AlpacaEquatorialCoordinateType::Other;

	VARIANT v1;
	HRESULT hResult = OlePropertyGet(pTelescopeDispatch, &v1, const_cast<wchar_t*>(LEquatorialSystem));

	if (FAILED(hResult))
	{
		qDebug() << "Could not get EquatorialCoordinateType for device: " << mAlpacaDeviceId;
		return AlpacaDevice::AlpacaEquatorialCoordinateType::Other;
	}

	return static_cast<AlpacaDevice::AlpacaEquatorialCoordinateType>(v1.intVal);
	*/
	// TODO: GET call
	return AlpacaDevice::AlpacaEquatorialCoordinateType::Other;
}

bool AlpacaDevice::doesRefraction()
{
	/*
	if (!mConnected) return false;

	VARIANT v1;
	HRESULT hResult = OlePropertyGet(pTelescopeDispatch, &v1, const_cast<wchar_t*>(LDoesRefraction));

	if (FAILED(hResult))
	{
		qDebug() << "Could not get DoesRefraction for device: " << mAlpacaDeviceId;
		return false;
	}

	return v1.boolVal == -1;
	*/
	// TODO: GET call
	return false;
}


AlpacaDevice::AlpacaCoordinates AlpacaDevice::getCoordinates()
{
	AlpacaDevice::AlpacaCoordinates coords = {0.,0.};

	/*
	if (!mConnected) return coords;

	VARIANT v1, v2;
	HRESULT hResult;

	hResult = OlePropertyGet(pTelescopeDispatch, &v1, const_cast<wchar_t*>(LRightAscension));
	if (FAILED(hResult)) qDebug() << "Could not get RightAscension for device: " << mAlpacaDeviceId;

	hResult = OlePropertyGet(pTelescopeDispatch, &v2, const_cast<wchar_t*>(LDeclination));
	if (FAILED(hResult)) qDebug() << "Could not get Declination for device: " << mAlpacaDeviceId;

	coords.RA = v1.dblVal;
	coords.DEC = v2.dblVal;
	*/

	// TODO: Use some GET call
	return coords;
}

QString AlpacaDevice::showDeviceChooser(QString previousDeviceId)
{
	/*
	VARIANT v1, v2;
	HRESULT hResult;
	IDispatch* chooserDispatch;
	BOOL initResult = OleInit(COINIT_APARTMENTTHREADED);

	if (!initResult) return previousDeviceId;
	hResult = OleCreateInstance(L"Alpaca.Utilities.Chooser", &chooserDispatch);

	if (FAILED(hResult)) return previousDeviceId;
	
	v1 = OleStringToVariant(const_cast<wchar_t*>(previousDeviceId.toStdWString().c_str()));

	hResult = OleMethodCall(chooserDispatch, &v2, const_cast<wchar_t*>(LChoose), 1, v1);

	if (FAILED(hResult)) return previousDeviceId;

	QString selectedDevice = QString::fromStdWString(v2.bstrVal);

	if (selectedDevice == "") return previousDeviceId;

	return QString::fromStdWString(v2.bstrVal);
	*/
	return "NOT IMPLEMENTED YET";
}

QString AlpacaDevice::getDeviceURL(QString &command)
{
	return QString("http://%1:%2/api/v%3/%4/%5/%6").arg(alpacaHost, QString::number(alpacaPort), ProtocolVersion, DeviceType, QString::number(alpacaDeviceNumber), command);
}

void AlpacaDevice::alpacaGET()
{
	QString clientArgs=QString("clientid=%2&clienttransactionid=%2").arg(QString::number(clientId), QString::number(clientTransactionId));
}

const QString AlpacaDevice::LSlewToCoordinatesAsync = QStringLiteral("SlewToCoordinatesAsync");
const QString AlpacaDevice::LSyncToCoordinates = QStringLiteral("SyncToCoordinates");
const QString AlpacaDevice::LAbortSlew = QStringLiteral("AbortSlew");
const QString AlpacaDevice::LConnected = QStringLiteral("Connected");
const QString AlpacaDevice::LAtPark = QStringLiteral("AtPark");
const QString AlpacaDevice::LEquatorialSystem = QStringLiteral("EquatorialSystem");
const QString AlpacaDevice::LDoesRefraction = QStringLiteral("DoesRefraction");
const QString AlpacaDevice::LRightAscension = QStringLiteral("RightAscension");
const QString AlpacaDevice::LDeclination = QStringLiteral("Declination");
const QString AlpacaDevice::LChoose = QStringLiteral("Choose");

const QString AlpacaDevice::ProtocolVersion = QStringLiteral("1");
const QString AlpacaDevice::DeviceType = QStringLiteral("telescope");
