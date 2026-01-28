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

#ifndef TELESCOPECLIENTALPACA_HPP
#define TELESCOPECLIENTALPACA_HPP

#include <QNetworkAccessManager>

#include "TelescopeControl.hpp"
#include "TelescopeClient.hpp"
#include "AlpacaDevice.hpp"


class TelescopeClientAlpaca : public TelescopeClient
{
	Q_OBJECT

public:
	TelescopeClientAlpaca(const QString& name = "Alpaca", const QString& params = QString(), TelescopeControl::Equinox eq = TelescopeControl::EquinoxJ2000);
	~TelescopeClientAlpaca() override;

	Vec3d getJ2000EquatorialPos(const StelCore *core) const override;
	void telescopeGoto(const Vec3d &j2000Pos, StelObjectP selectObject) override;
	void telescopeSync(const Vec3d& j2000Pos, StelObjectP selectObject) override;
	bool isTelescopeSyncSupported() const override {return true;}
	void telescopeAbortSlew() override;
	bool isAbortSlewSupported() const override {return true;}
	bool isConnected() const override;
	bool hasKnownPosition() const override;
	static bool useJNow(AlpacaDevice::AlpacaEquatorialCoordinateType coordinateType, bool AlpacaUseDeviceEqCoordType, TelescopeControl::Equinox equinox);


private:
	bool prepareCommunication() override;
	void performCommunication() override;

	QNetworkAccessManager *qNAM;

	AlpacaDevice::AlpacaCoordinates j2000PosToAlpacaCoord(const Vec3d& j2000Pos);
	QString mAlpacaDeviceId;
	bool mAlpacaUseDeviceEqCoordType;
	AlpacaDevice* mAlpacaDevice;
	AlpacaDevice::AlpacaEquatorialCoordinateType mCoordinateType;
	bool mDoesRefraction; //! Telescope supports refraction correction. TODO: Check this on connecting.
	TelescopeControl::Equinox mEquinox;
	AlpacaDevice::AlpacaCoordinates mLastCoord;
	Vec3d mInterpolatedPosition;
	Vec3d mCurrentTargetPosition;
	qint64 mLastUpdateTime;
};

#endif // TELESCOPECLIENTALPACA_HPP
