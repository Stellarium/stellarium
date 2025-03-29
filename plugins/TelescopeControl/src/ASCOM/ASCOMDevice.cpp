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
#include <comdef.h>
#include <QTimer>
//#include <qwaitcondition.h>
#include <QMessageBox>
#include "StelTranslator.hpp"

ASCOMDevice::ASCOMDevice(QObject* parent, QString ascomDeviceId) : QObject(parent),
	mAscomDeviceId(ascomDeviceId),
	connectionRetries(0)
{
}

// FIXME! ASCOM7 deprecates writing to the LConnected property. One shall use methods Connect()/Disconnect() instead,
// and detect the state per "Connecting" property.
void ASCOMDevice::connect()
{
	if (mConnected) return; // true;

	BOOL initResult = OleInit(COINIT_APARTMENTTHREADED);
	HRESULT hResult = OleCreateInstance(reinterpret_cast<const wchar_t*>(mAscomDeviceId.toStdWString().c_str()), &pTelescopeDispatch);

	if (!initResult || FAILED(hResult))
	{
		qCritical() << "Initialization failed for device: " << mAscomDeviceId;
		QMessageBox::critical(nullptr, q_("ERROR!"), q_("Initialization failed for device: ")+mAscomDeviceId);

		return;
	}

	// Initiate asynchronous Connect() according to ASCOM7
	hResult = OleMethodCall(pTelescopeDispatch, nullptr, const_cast<wchar_t*>(LConnect), 0);
	if (FAILED(hResult))
	{
		qCritical() << "Could not send Connect signal to device: " << mAscomDeviceId;
		QMessageBox::critical(nullptr, q_("ERROR!"), q_("Could not send Connect signal to device: ")+mAscomDeviceId);
		return;
	}
	connectionRetries=10;
	QObject::connect(&connectionTimer, &QTimer::timeout, this, &ASCOMDevice::tryFinishConnect);
	connectionTimer.start(250); // 1/4 s interval
}

void ASCOMDevice::tryFinishConnect()
{
	qDebug() << "ASCOMDevice::tryFinishConnect()";

	if (isConnecting())
	{
		if (--connectionRetries <= 0)
		{
			// Give up connecting!
			connectionTimer.stop();
			QObject::disconnect(&connectionTimer, &QTimer::timeout, this, &ASCOMDevice::tryFinishConnect);
			qCritical() << "Could not establish connection to device: " << mAscomDeviceId;
			QMessageBox::critical(nullptr, q_("ERROR!"), q_("Could not establish connection to device: ")+mAscomDeviceId);
		}
	}
	else // connection has probably been established. Read value for certainty.
	{
		connectionTimer.stop();
		QObject::disconnect(&connectionTimer, &QTimer::timeout, this, &ASCOMDevice::tryFinishConnect);
		mConnected=isDeviceConnected();
	}
}

void ASCOMDevice::tryFinishDisconnect()
{
	qDebug() << "ASCOMDevice::tryFinishDisconnect()";
	if (isConnecting())
	{
		if (--connectionRetries <= 0)
		{
			// Give up disconnecting! Is that at all useful?
			connectionTimer.stop();
			QObject::disconnect(&connectionTimer, &QTimer::timeout, this, &ASCOMDevice::tryFinishDisconnect);
			qCritical() << "Could not send Disconnect signal to device: " << mAscomDeviceId;
			// TODO: Show a screen panel?
			QMessageBox::critical(nullptr, q_("ERROR!"), q_("Could not cleanly disconnect from device: ")+mAscomDeviceId);
		}
	}
	else // disconnection has been successful. Read value for certainty.
	{
		connectionTimer.stop();
		QObject::disconnect(&connectionTimer, &QTimer::timeout, this, &ASCOMDevice::tryFinishDisconnect);
		mConnected=false;
		pTelescopeDispatch->Release();
	}
}



//void ASCOMDevice::disconnect()
//{
//	if (!mConnected) return; // true;
//
//	VARIANT v1 = OleBoolToVariant(FALSE);
//	HRESULT hResult = OlePropertyPut(pTelescopeDispatch, nullptr, const_cast<wchar_t*>(LConnected), 1, v1);
//
//	if (FAILED(hResult))
//	{
//		qDebug() << "Could not disconnect device: " << mAscomDeviceId;
//		return; // false;
//	}
//
//	pTelescopeDispatch->Release();  // TODO: Move that to tryFinishDisconnect
//
//	mConnected = false;
//	return; // true;
//}


void ASCOMDevice::disconnect()
{
	if (mConnected) return; // true;


	// Initiate asynchronous Disconnect() according to ASCOM7
	HRESULT hResult = OleMethodCall(pTelescopeDispatch, nullptr, const_cast<wchar_t*>(LDisconnect), 0);
	if (FAILED(hResult))
	{
		qCritical() << "Could not send Disconnect signal to device: " << mAscomDeviceId;
		// TODO: Show a screen panel?
		return;
	}
	connectionRetries=10;
	QObject::connect(&connectionTimer, &QTimer::timeout, this, &ASCOMDevice::tryFinishDisconnect);
	connectionTimer.start(250); // 1/4 s interval
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

	HRESULT hResult = OleMethodCall(pTelescopeDispatch, nullptr, const_cast<wchar_t*>(LSlewToCoordinatesAsync), 2, v1, v2);
	if (FAILED(hResult))
	{
		qDebug() << "Slew failed for device: " << mAscomDeviceId;
	}
}

void ASCOMDevice::syncToCoordinates(ASCOMCoordinates coords)
{
	if (!mConnected) return;

	VARIANT v1 = OleDoubleToVariant(coords.RA);
	VARIANT v2 = OleDoubleToVariant(coords.DEC);

	HRESULT hResult = OleMethodCall(pTelescopeDispatch, nullptr, const_cast<wchar_t*>(LSyncToCoordinates), 2, v1, v2);
	if (FAILED(hResult))
	{
		qDebug() << "Could not sync to coordinates for device: " << mAscomDeviceId;
	}
}

void ASCOMDevice::abortSlew()
{
	if (!mConnected) return;

	HRESULT hResult = OleMethodCall(pTelescopeDispatch, nullptr, const_cast<wchar_t*>(LAbortSlew));
	if (FAILED(hResult))
	{
		qCritical() << "Could not abort slew for device: " << mAscomDeviceId;
	}
}

bool ASCOMDevice::isConnected() const
{
	return mConnected;
}

bool ASCOMDevice::isDeviceConnected() const
{
	if (!mConnected) return false;

	VARIANT v1;
	HRESULT hResult = OlePropertyGet(pTelescopeDispatch, &v1, const_cast<wchar_t*>(LConnected));

	if (FAILED(hResult))
	{
		qDebug() << "Could not get connected state for device: " << mAscomDeviceId;
		return false;
	}

	return v1.boolVal == VARIANT_TRUE;
}

bool ASCOMDevice::isConnecting() const
{
	VARIANT v1;
	HRESULT hResult = OlePropertyGet(pTelescopeDispatch, &v1, const_cast<wchar_t*>(LConnecting));

	if (FAILED(hResult))
	{
		qDebug() << "Could not get connecting state for device: " << mAscomDeviceId;
		return false;
	}

	return v1.boolVal == VARIANT_TRUE;
}

bool ASCOMDevice::isParked() const
{
	if (!mConnected) return true;

	VARIANT v1;
	HRESULT hResult = OlePropertyGet(pTelescopeDispatch, &v1, const_cast<wchar_t*>(LAtPark));

	if (FAILED(hResult))
	{
		qDebug() << "Could not get AtPark state for device: " << mAscomDeviceId;
		return false;
	}

	return v1.boolVal == VARIANT_TRUE;
}


ASCOMDevice::ASCOMEquatorialCoordinateType ASCOMDevice::getEquatorialCoordinateType()
{
	if (!mConnected) return ASCOMDevice::ASCOMEquatorialCoordinateType::Other;

	VARIANT v1;
	HRESULT hResult = OlePropertyGet(pTelescopeDispatch, &v1, const_cast<wchar_t*>(LEquatorialSystem));

	if (FAILED(hResult))
	{
		qDebug() << "Could not get EquatorialCoordinateType for device: " << mAscomDeviceId;
		return ASCOMDevice::ASCOMEquatorialCoordinateType::Other;
	}

	return static_cast<ASCOMDevice::ASCOMEquatorialCoordinateType>(v1.intVal);
}

bool ASCOMDevice::doesRefraction()
{
	if (!mConnected) return false;

	VARIANT v1;
	HRESULT hResult = OlePropertyGet(pTelescopeDispatch, &v1, const_cast<wchar_t*>(LDoesRefraction));

	if (FAILED(hResult))
	{
		qDebug() << "Could not get DoesRefraction for device: " << mAscomDeviceId;
		return false;
	}

	return v1.boolVal == VARIANT_TRUE;
}


ASCOMDevice::ASCOMCoordinates ASCOMDevice::getCoordinates()
{
	ASCOMDevice::ASCOMCoordinates coords;

	if (!mConnected) return coords;

	VARIANT v1, v2;
	HRESULT hResult;

	hResult = OlePropertyGet(pTelescopeDispatch, &v1, const_cast<wchar_t*>(LRightAscension));
	if (FAILED(hResult)) qDebug() << "Could not get RightAscension for device: " << mAscomDeviceId;

	hResult = OlePropertyGet(pTelescopeDispatch, &v2, const_cast<wchar_t*>(LDeclination));
	if (FAILED(hResult)) qDebug() << "Could not get Declination for device: " << mAscomDeviceId;

	coords.RA = v1.dblVal;
	coords.DEC = v2.dblVal;

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
const wchar_t* ASCOMDevice::LAbortSlew  = L"AbortSlew";
const wchar_t* ASCOMDevice::LConnected  = L"Connected";
const wchar_t* ASCOMDevice::LConnecting = L"Connecting";
const wchar_t* ASCOMDevice::LConnect    = L"Connect";
const wchar_t* ASCOMDevice::LDisconnect = L"Disconnect";
const wchar_t* ASCOMDevice::LAtPark     = L"AtPark";
const wchar_t* ASCOMDevice::LEquatorialSystem = L"EquatorialSystem";
const wchar_t* ASCOMDevice::LDoesRefraction = L"DoesRefraction";
const wchar_t* ASCOMDevice::LRightAscension = L"RightAscension";
const wchar_t* ASCOMDevice::LDeclination = L"Declination";
const wchar_t* ASCOMDevice::LChoose = L"Choose";
