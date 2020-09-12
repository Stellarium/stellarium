/*
 * Copyright (C) 2017 Alessandro Siniscalchi <asiniscalchi@gmail.com>
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

#ifndef INDICONNECTION_HPP
#define INDICONNECTION_HPP

#include <QObject>
#include "indibase/baseclient.h"

#include <mutex>
#include <QStringList>

class INDIConnection final : public QObject, public INDI::BaseClient
{
	Q_OBJECT

public:
	static const int SLEW_STOP;

	struct Coordinates
	{
		double RA = 0.0;
		double DEC = 0.0;
		bool operator==(const Coordinates &other) const;
		bool operator!=(const Coordinates &other) const;
	};

	INDIConnection(QObject* parent = Q_NULLPTR);
	INDIConnection(const INDIConnection& that) = delete;

	Coordinates position() const;
	void setPosition(Coordinates coords);
	void syncPosition(Coordinates coords);
	bool isDeviceConnected() const;
	const QStringList devices() const;
	void moveNorth(int speed);
	void moveEast(int speed);
	void moveSouth(int speed);
	void moveWest(int speed);

signals:
	void newDeviceReceived(QString name);
	void removeDeviceReceived(QString name);
	void serverConnectedReceived();
	void serverDisconnectedReceived(int exit_code);
	void speedChanged(int speed);

private:
	void setSpeed(int speed);

	mutable std::mutex mMutex;
	INDI::BaseDevice* mTelescope = Q_NULLPTR;	
	Coordinates mCoordinates;
	QStringList mDevices;

public: // from INDI::BaseClient
	void newDevice(INDI::BaseDevice *dp) override;
	void removeDevice(INDI::BaseDevice *dp) override;
	void newProperty(INDI::Property *property) override;
	void removeProperty(INDI::Property *property) override;
	void newBLOB(IBLOB *bp) override;
	void newSwitch(ISwitchVectorProperty *svp) override;
	void newNumber(INumberVectorProperty *nvp) override;
	void newText(ITextVectorProperty *tvp) override;
	void newLight(ILightVectorProperty *lvp) override;
	void newMessage(INDI::BaseDevice *dp, int messageID) override;
	void serverConnected() override;
	void serverDisconnected(int exit_code) override;
	void unParkTelescope();
	//void parkTelescope();
};

#endif // INDICONNECTION_HPP
