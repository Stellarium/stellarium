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

#ifndef TELESCOPECLIENTASCOM_HPP
#define TELESCOPECLIENTASCOM_HPP

#include "TelescopeControl.hpp"
#include "TelescopeClient.hpp"
#include "ASCOMDevice.hpp"


class TelescopeClientASCOM : public TelescopeClient
{
	Q_OBJECT

public:
	TelescopeClientASCOM(const QString& name = "ASCOM", const QString& params = QString(), TelescopeControl::Equinox eq = TelescopeControl::EquinoxJ2000);
	~TelescopeClientASCOM() override;

	Vec3d getJ2000EquatorialPos(const StelCore *core) const override;
	void move(double angle, double speed) override;
	void telescopeGoto(const Vec3d &j2000Pos, StelObjectP selectObject) override;
	void telescopeSync(const Vec3d& j2000Pos, StelObjectP selectObject) override;
	void telescopeAbortSlew() override;
	bool isConnected() const override;
	bool hasKnownPosition() const override;
	static bool useJNow(ASCOMDevice::ASCOMEquatorialCoordinateType coordinateType, bool ascomUseDeviceEqCoordType, TelescopeControl::Equinox equinox);


private:
	bool prepareCommunication() override;
	void performCommunication() override;

	ASCOMDevice::ASCOMCoordinates j2000PosToAscomCoord(const Vec3d& j2000Pos);
	QString mAscomDeviceId;
	bool mAscomUseDeviceEqCoordType;
	ASCOMDevice* mAscomDevice;
	ASCOMDevice::ASCOMEquatorialCoordinateType mCoordinateType;
	bool mDoesRefraction;
	TelescopeControl::Equinox mEquinox;
	ASCOMDevice::ASCOMCoordinates mLastCoord;
	Vec3d mInterpolatedPosition;
	Vec3d mCurrentTargetPosition;
	qint64 mLastUpdateTime;
};

#endif // TELESCOPECLIENTASCOM_HPP
