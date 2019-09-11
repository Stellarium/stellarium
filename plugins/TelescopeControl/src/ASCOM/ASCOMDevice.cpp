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
#include <atlcomcli.h>
#include <comdef.h>

ASCOMDevice::ASCOMDevice(QObject* parent, QString ascomDeviceId) : QObject(parent)
{
	HRESULT hResult;
	BOOL initResult;

	mAscomDeviceId = ascomDeviceId;
	initResult = OleInit(COINIT_APARTMENTTHREADED);
	hResult = OleCreateInstance((const wchar_t*)mAscomDeviceId.utf16(), &pTelescopeDispatch);

	if (FAILED(hResult)) {
		qDebug() << "Initialization failed for device: " << mAscomDeviceId;
	};
}

bool ASCOMDevice::connect()
{
	VARIANT v1;
	HRESULT hResult;
	
	v1 = OleBoolToVariant(TRUE);

	hResult = OlePropertyPut(pTelescopeDispatch, NULL, L"Connected", 1, v1);

	if (FAILED(hResult))
	{
		qDebug() << "Could not connect to device: " << mAscomDeviceId;
		return false;
	}
	
	return true;
}

bool ASCOMDevice::disconnect()
{
	VARIANT v1;
	HRESULT hResult;

	v1 = OleBoolToVariant(FALSE);

	hResult = OlePropertyPut(pTelescopeDispatch, NULL, L"Connected", 1, v1);

	if (FAILED(hResult))
	{
		qDebug() << "Could not disconnect device: " << mAscomDeviceId;
		return false;
	};

	return true;
}

ASCOMDevice::ASCOMCoordinates ASCOMDevice::position() const
{
	return mCoordinates;
}


void ASCOMDevice::slewToCoordinates(ASCOMDevice::ASCOMCoordinates coords) 
{
	VARIANT v1, v2;
	HRESULT hResult;

	v1 = OleDoubleToVariant(coords.RA);
	v2 = OleDoubleToVariant(coords.DEC);

	hResult = OleMethodCall(pTelescopeDispatch, NULL, L"SlewToCoordinatesAsync", 2, v1, v2);
}

void ASCOMDevice::syncToCoordinates(ASCOMCoordinates coords)
{
	VARIANT v1, v2;
	HRESULT hResult;

	v1 = OleDoubleToVariant(coords.RA);
	v2 = OleDoubleToVariant(coords.DEC);

	hResult = OleMethodCall(pTelescopeDispatch, NULL, L"SyncToCoordinates", 2, v1, v2);
	if (FAILED(hResult))
	{
		qDebug() << "Could not sync to coordinates for device: " << mAscomDeviceId;
	}
}

void ASCOMDevice::abortSlew()
{
	HRESULT hResult;
	hResult = OleMethodCall(pTelescopeDispatch, NULL, L"AbortSlew");
	if (FAILED(hResult))
	{
		qDebug() << "Could not abort slew for device: " << mAscomDeviceId;
	}
}

bool ASCOMDevice::isDeviceConnected() const
{
	VARIANT v1;
	HRESULT hResult;

	hResult = OlePropertyGet(pTelescopeDispatch, &v1, L"Connected");

	if (FAILED(hResult))
	{
		qDebug() << "Could not get connected state for device: " << mAscomDeviceId;
		return false;
	}

	return v1.boolVal == -1;
}

bool ASCOMDevice::isParked() const
{
	VARIANT v1;
	HRESULT hResult;

	hResult = OlePropertyGet(pTelescopeDispatch, &v1, L"AtPark");

	if (FAILED(hResult))
	{
		qDebug() << "Could not get AtPark state for device: " << mAscomDeviceId;
		return false;
	}

	return v1.boolVal == -1;
}


ASCOMDevice::ASCOMEquatorialCoordinateType ASCOMDevice::getEquatorialCoordinateType()
{
	VARIANT v1;
	HRESULT hResult;

	hResult = OlePropertyGet(pTelescopeDispatch, &v1, L"EquatorialSystem");

	if (FAILED(hResult))
	{
		qDebug() << "Could not get EquatorialCoordinateType for device: " << mAscomDeviceId;
		return ASCOMDevice::ASCOMEquatorialCoordinateType::Other;
	}

	return static_cast<ASCOMDevice::ASCOMEquatorialCoordinateType>(v1.intVal);
}

bool ASCOMDevice::doesRefraction()
{
	VARIANT v1;
	HRESULT hResult;

	hResult = OlePropertyGet(pTelescopeDispatch, &v1, L"DoesRefraction");

	if (FAILED(hResult))
	{
		qDebug() << "Could not get DoesRefraction for device: " << mAscomDeviceId;
		return false;
	}

	return v1.boolVal == -1;
}


ASCOMDevice::ASCOMCoordinates ASCOMDevice::getCoordinates()
{
	VARIANT v1, v2;
	HRESULT hResult;

	hResult = OlePropertyGet(pTelescopeDispatch, &v1, L"RightAscension");
	if (FAILED(hResult)) qDebug() << "Could not get RightAscension for device: " << mAscomDeviceId;

	hResult = OlePropertyGet(pTelescopeDispatch, &v2, L"Declination");
	if (FAILED(hResult)) qDebug() << "Could not get Declination for device: " << mAscomDeviceId;

	ASCOMDevice::ASCOMCoordinates coords;
	coords.RA = v1.dblVal;
	coords.DEC = v2.dblVal;

	return coords;
}

QString ASCOMDevice::showDeviceChooser()
{
	VARIANT v1, v2;
	HRESULT hResult;
	IDispatch* chooserDispatch;
	BOOL initResult;

	initResult = OleInit(COINIT_APARTMENTTHREADED);
	// TODO: Better error management?
	if (!initResult) return NULL;
	hResult = OleCreateInstance(L"ASCOM.Utilities.Chooser", &chooserDispatch);

	// TODO: Better error management?
	if (FAILED(hResult)) return NULL;

	v1 = OleStringToVariant(L"");

	hResult = OleMethodCall(chooserDispatch, &v2, L"Choose", 1, v1);

	return QString::fromStdWString(v2.bstrVal);
}
