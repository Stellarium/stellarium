/*
 * Copyright (C) 2019 Gion Kunz <gion.kunz@gmail.com>
 * Copyright (C) 2019 Alexander Wolf <alex.v.wolf@gmail.com>
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

#include "testTelescopeControl_ASCOM.hpp"

void TestTelescopeControl_ASCOM::useJNowShouldDetermineCorrectly()
{
	// Given
	const ASCOMDevice::ASCOMEquatorialCoordinateType type = ASCOMDevice::ASCOMEquatorialCoordinateType::Topocentric;
	const bool useDeviceEqCoordType = true;
	const Equinox stellariumEquinox = Equinox::EquinoxJ2000;

	// When
	const bool jnow = useJNow(type, useDeviceEqCoordType, stellariumEquinox);

	// Then
	QVERIFY(jnow == true);
}

void TestTelescopeControl_ASCOM::useJNowShouldUseStellariumEquinoxWhenRequested()
{
	// Given
	const ASCOMDevice::ASCOMEquatorialCoordinateType type = ASCOMDevice::ASCOMEquatorialCoordinateType::Topocentric;
	const bool useDeviceEqCoordType = false;
	const Equinox stellariumEquinox = Equinox::EquinoxJ2000;

	// When
	const bool jnow = useJNow(type, useDeviceEqCoordType, stellariumEquinox);

	// Then
	QVERIFY(jnow == false);
}

void TestTelescopeControl_ASCOM::useJNowShouldUseJNowOnUnknownASCOM()
{
	// Given
	const ASCOMDevice::ASCOMEquatorialCoordinateType type = ASCOMDevice::ASCOMEquatorialCoordinateType::Other;
	const bool useDeviceEqCoordType = true;
	const Equinox stellariumEquinox = Equinox::EquinoxJ2000;

	// When
	const bool jnow = useJNow(type, useDeviceEqCoordType, stellariumEquinox);

	// Then
	QVERIFY(jnow == true);
}

void TestTelescopeControl_ASCOM::areSimilarShouldCompareCorrectly()
{
	// Give
	const double a = 0.00000000000001;
	const double b = 0.00000000000001;

	// When
	const bool isSimilar = areSimilar(a, b);

	// Then
	QVERIFY(isSimilar == true);
}

void TestTelescopeControl_ASCOM::areSimilarShouldShowSlightErrors()
{
	// Give
	const double a = 0.00000000000001;
	const double b = 0.0000000000001;

	// When
	const bool isSimilar = areSimilar(a, b);

	// Then
	QVERIFY(isSimilar == false);
}

void TestTelescopeControl_ASCOM::ascomDeviceShouldFailToInitializeWithInvalidDevice()
{
	// Given
	ASCOMDevice* device;

	// When
	device = new ASCOMDevice(Q_NULLPTR, Q_NULLPTR);

	// Then
	QVERIFY(device->connect() == false);	
}

QTEST_MAIN(TestTelescopeControl_ASCOM)
