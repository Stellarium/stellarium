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

#include "ASCOMDevice.hpp"
#include "TelescopeControl.hpp"
#include <comdef.h>

ASCOMDevice::ASCOMDevice(QObject* parent, QString ascomDeviceId) : QObject(parent),
	mAscomDeviceId(ascomDeviceId)
{}

bool ASCOMDevice::connect()
{
	if (mConnected) return true;

	BOOL initResult = OleInit(COINIT_APARTMENTTHREADED);
	HRESULT hResult = OleCreateInstance(reinterpret_cast<const wchar_t*>(mAscomDeviceId.toStdWString().c_str()), &pTelescopeDispatch);

	if (!initResult || FAILED(hResult))
	{
		qCWarning(Telescopes) << "Initialization failed for device: " << mAscomDeviceId;
		return false;
	}

	VARIANT v1 = OleBoolToVariant(TRUE);

	hResult = OlePropertyPut(pTelescopeDispatch, nullptr, const_cast<wchar_t*>(LConnected), 1, v1);

	if (FAILED(hResult))
	{
		qCWarning(Telescopes) << "Could not connect to device: " << mAscomDeviceId;
		return false;
	}

	// With ASCOM 7.1 there is a problem reaching the Properties panel of the simulators.
	// Before sending a slew, telescopes must be set to tracking.
	qCDebug(Telescopes) << "This device is tracking:" << isTracking();
	if (!isTracking())
	{
		VARIANT vT = OleBoolToVariant(TRUE);

		hResult = OlePropertyPut(pTelescopeDispatch, nullptr, const_cast<wchar_t*>(LTracking), 1, vT);

		if (FAILED(hResult))
		{
			qCWarning(Telescopes) << "Could not enable Tracking on device: " << mAscomDeviceId;
			return false;
		}
	}
	mConnected = true;
	return true;
}

bool ASCOMDevice::disconnect()
{
	if (!mConnected) return true;

	VARIANT v1 = OleBoolToVariant(FALSE);
	HRESULT hResult = OlePropertyPut(pTelescopeDispatch, nullptr, const_cast<wchar_t*>(LConnected), 1, v1);

	if (FAILED(hResult))
	{
		qCWarning(Telescopes) << "Could not disconnect device: " << mAscomDeviceId;
		return false;
	}
	
	pTelescopeDispatch->Release();

	mConnected = false;
	return true;
}

ASCOMDevice::ASCOMCoordinates ASCOMDevice::position() const
{
	return mCoordinates;
}


void ASCOMDevice::slewToCoordinates(ASCOMDevice::ASCOMCoordinates coords) 
{
	if (!mConnected) return;

	VARIANT v1 = OleDoubleToVariant(coords.RA);
	VARIANT v2 = OleDoubleToVariant(coords.DEC);

	qCDebug(Telescopes) << "Slewing to coordinates" << variantToQstring(v1) << " dec " << variantToQstring(v2);
	qCDebug(Telescopes) << "Slewing to coordinates" << QString::number(variantToDouble(v1)) << " dec " << QString::number(variantToDouble(v2));

	HRESULT hResult = OleMethodCall(pTelescopeDispatch, nullptr, const_cast<wchar_t*>(LSlewToCoordinatesAsync), 2, v1, v2);
	if (FAILED(hResult))
	{
		qCWarning(Telescopes) << "Slew failed for device: " << mAscomDeviceId;
	}
}

void ASCOMDevice::syncToCoordinates(ASCOMCoordinates coords)
{
	if (!mConnected) return;

	VARIANT v1 = OleDoubleToVariant(coords.RA);
	VARIANT v2 = OleDoubleToVariant(coords.DEC);

	qCDebug(Telescopes) << "Syncing to coordinates" << variantToQstring(v1) << " dec " << variantToQstring(v2);
	HRESULT hResult = OleMethodCall(pTelescopeDispatch, nullptr, const_cast<wchar_t*>(LSyncToCoordinates), 2, v1, v2);
	if (FAILED(hResult))
	{
		qCWarning(Telescopes) << "Could not sync to coordinates for device: " << mAscomDeviceId;
	}
}

void ASCOMDevice::abortSlew()
{
	if (!mConnected) return;

	HRESULT hResult = OleMethodCall(pTelescopeDispatch, nullptr, const_cast<wchar_t*>(LAbortSlew));
	if (FAILED(hResult))
	{
		qCCritical(Telescopes) << "Could not abort slew for device: " << mAscomDeviceId;
	}
}

bool ASCOMDevice::isDeviceConnected() const
{
	if (!mConnected) return false;

	VARIANT v1;
	HRESULT hResult = OlePropertyGet(pTelescopeDispatch, &v1, const_cast<wchar_t*>(LConnected));
	// TODO: if possible, change this. It fires once per frame in draw()!
	//qCDebug(Telescopes) << "ASCOMDevice::isDeviceConnected reports" << variantToQstring(v1);

	if (FAILED(hResult))
	{
		qCWarning(Telescopes) << "Could not get connected state for device: " << mAscomDeviceId;
		return false;
	}

	return v1.boolVal == -1;
}

bool ASCOMDevice::isParked() const
{
	if (!mConnected) return true;

	VARIANT v1;
	HRESULT hResult = OlePropertyGet(pTelescopeDispatch, &v1, const_cast<wchar_t*>(LAtPark));
	qCDebug(Telescopes) << "ASCOMDevice::isParked reports" << variantToQstring(v1);

	if (FAILED(hResult))
	{
		qCWarning(Telescopes) << "Could not get AtPark state for device: " << mAscomDeviceId;
		return false;
	}

	return v1.boolVal == -1;
}

bool ASCOMDevice::isTracking() const
{
	if (!mConnected) return true;

	VARIANT v1;
	HRESULT hResult = OlePropertyGet(pTelescopeDispatch, &v1, const_cast<wchar_t*>(LTracking));
	qCDebug(Telescopes) << "ASCOMDevice::isTracking reports" << variantToQstring(v1);

	if (FAILED(hResult))
	{
		qCWarning(Telescopes) << "Could not get Tracking state for device: " << mAscomDeviceId;
		return false;
	}

	return v1.boolVal == -1;
}

ASCOMDevice::ASCOMEquatorialCoordinateType ASCOMDevice::getEquatorialCoordinateType()
{
	if (!mConnected) return ASCOMDevice::ASCOMEquatorialCoordinateType::Other;

	VARIANT v1;
	HRESULT hResult = OlePropertyGet(pTelescopeDispatch, &v1, const_cast<wchar_t*>(LEquatorialSystem));
	qCDebug(Telescopes) << "ASCOMDevice::getEquatorialCoordinateType reports" << variantToQstring(v1);

	if (FAILED(hResult))
	{
		qCWarning(Telescopes) << "Could not get EquatorialCoordinateType for device: " << mAscomDeviceId;
		return ASCOMDevice::ASCOMEquatorialCoordinateType::Other;
	}

	return static_cast<ASCOMDevice::ASCOMEquatorialCoordinateType>(v1.intVal);
}

bool ASCOMDevice::doesRefraction()
{
	if (!mConnected) return false;

	VARIANT v1;
	HRESULT hResult = OlePropertyGet(pTelescopeDispatch, &v1, const_cast<wchar_t*>(LDoesRefraction));
	qCDebug(Telescopes) << "ASCOMDevice::doesRefraction reports" << variantToQstring(v1);

	if (FAILED(hResult))
	{
		qCWarning(Telescopes) << "Could not get DoesRefraction for device: " << mAscomDeviceId;
		return false;
	}

	return v1.boolVal == -1;
}


ASCOMDevice::ASCOMCoordinates ASCOMDevice::getCoordinates()
{
	ASCOMDevice::ASCOMCoordinates coords;

	if (!mConnected) return coords;

	VARIANT v1, v2;
	HRESULT hResult;

	hResult = OlePropertyGet(pTelescopeDispatch, &v1, const_cast<wchar_t*>(LRightAscension));
	if (FAILED(hResult)) qCWarning(Telescopes) << "Could not get RightAscension for device: " << mAscomDeviceId;

	hResult = OlePropertyGet(pTelescopeDispatch, &v2, const_cast<wchar_t*>(LDeclination));
	if (FAILED(hResult)) qCWarning(Telescopes) << "Could not get Declination for device: " << mAscomDeviceId;

	coords.RA = v1.dblVal;
	coords.DEC = v2.dblVal;
	qCDebug(Telescopes) << "ASCOMDevice::getCoordinates(): RA/Dec:" << QString::number(coords.RA) << "/" << QString::number(coords.DEC);

	return coords;
}

QString ASCOMDevice::showDeviceChooser(QString previousDeviceId)
{
	VARIANT v1, v2;
	HRESULT hResult;
	IDispatch* chooserDispatch;
	BOOL initResult = OleInit(COINIT_APARTMENTTHREADED);

	if (!initResult) return previousDeviceId;
	hResult = OleCreateInstance(L"ASCOM.Utilities.Chooser", &chooserDispatch);

	if (FAILED(hResult)) return previousDeviceId;
	
	v1 = OleStringToVariant(const_cast<wchar_t*>(previousDeviceId.toStdWString().c_str()));

	hResult = OleMethodCall(chooserDispatch, &v2, const_cast<wchar_t*>(LChoose), 1, v1);

	if (FAILED(hResult)) return previousDeviceId;

	QString selectedDevice = QString::fromStdWString(v2.bstrVal);

	if (selectedDevice == "") return previousDeviceId;

	return QString::fromStdWString(v2.bstrVal);
}

const wchar_t* ASCOMDevice::LSlewToCoordinatesAsync = L"SlewToCoordinatesAsync";
const wchar_t* ASCOMDevice::LSyncToCoordinates = L"SyncToCoordinates";
const wchar_t* ASCOMDevice::LAbortSlew = L"AbortSlew";
const wchar_t* ASCOMDevice::LConnected = L"Connected";
const wchar_t* ASCOMDevice::LAtPark = L"AtPark";
const wchar_t* ASCOMDevice::LEquatorialSystem = L"EquatorialSystem";
const wchar_t* ASCOMDevice::LDoesRefraction = L"DoesRefraction";
const wchar_t* ASCOMDevice::LRightAscension = L"RightAscension";
const wchar_t* ASCOMDevice::LDeclination = L"Declination";
const wchar_t* ASCOMDevice::LChoose = L"Choose";
const wchar_t* ASCOMDevice::LTracking = L"Tracking";
