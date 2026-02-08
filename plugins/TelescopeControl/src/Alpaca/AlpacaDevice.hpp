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

#ifndef ALPACA_DEVICE_HPP
#define ALPACA_DEVICE_HPP

#include <QObject>
#include <QStringList>


//! Implementation of an AlpacaDevice that actually only can be an Alpaca telescope
//! TODO Maybe rename to AlpacaTelescopeDevice?
class AlpacaDevice final : public QObject
{
	Q_OBJECT

public:
	struct AlpacaCoordinates
	{
		double RA = 0.0;
		double DEC = 0.0;
	};

	enum AlpacaEquatorialCoordinateType
	{
		Other, Topocentric, J2000, J2050, B1950
	};

	static QString showDeviceChooser(QString previousDeviceId = "");

	AlpacaDevice(QObject* parent = nullptr, QString AlpacaDeviceId = nullptr);
	
	bool isDeviceConnected() const; // Maybe not useful in Alpaca. isDeviceConfigured() or Active() may be useful.
	bool isParked() const;
	bool connect();
	bool disconnect();
	AlpacaCoordinates position() const;
	void slewToCoordinates(AlpacaCoordinates coords);
	void syncToCoordinates(AlpacaCoordinates coords);
	void abortSlew();
	AlpacaEquatorialCoordinateType getEquatorialCoordinateType();
	bool doesRefraction();
	AlpacaCoordinates getCoordinates();

signals:
	void deviceConnected();
	void deviceDisconnected();

private:
	//IDispatch* pTelescopeDispatch; // from OLE
	//bool mConnected = false; // No longer meaningful. RESTful, no state...
	QString alpacaHost; //! "mytelescope.net" or "127.0.0.1"
	int alpacaPort; //! port used for communication. Is returned via discovery protocol. 0 means invalid.
	uint32_t alpacaDeviceNumber; //! Only the telescope number. Get during discovery

	uint32_t clientId;            //! User configurable before first connection. Send as clientid in each command.
	uint32_t clientTransactionId; //! operation counter. send as clienttransactionid, raise per command given.

	AlpacaCoordinates mCoordinates; //! TODO: No longer relevant, as coordinates are read on the fly, right?

	//! Return the URL path part for this telescope, like /api/v1/telescope/0
	QString getDeviceURL(QString &command);

	void alpacaGET();
	void alpacaPUT();

	static const QString LSlewToCoordinatesAsync;
	static const QString LSyncToCoordinates;
	static const QString LAbortSlew;
	static const QString LConnected;
	static const QString LAtPark;
	static const QString LEquatorialSystem;
	static const QString LDoesRefraction;
	static const QString LRightAscension;
	static const QString LDeclination;
	static const QString LChoose;

	static const QString ProtocolVersion;
	static const QString DeviceType;
};

#endif // ALPACA_DEVICE_HPP
