/*
 * Copyright (C) 2019 Gion Kunz <gion.kunz@gmail.com> (ASCOM)
 * Copyright (C) 2019 Alexander Wolf <alex.v.wolf@gmail.com>
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

#ifndef TESTTELESCOPECONTROL_ALPACA_HPP
#define TESTTELESCOPECONTROL_ALPACA_HPP

#include <QtTest>

#include "TelescopeClientAlpaca.hpp"
#include "AlpacaDevice.hpp"

class TestTelescopeControl_Alpaca : public QObject
{
    Q_OBJECT

private slots:
	void useJNowShouldDetermineCorrectly();
	void useJNowShouldUseStellariumEquinoxWhenRequested();
	void useJNowShouldUseJNowOnUnknownAlpaca();
	void areSimilarShouldCompareCorrectly();
	void areSimilarShouldShowSlightErrors();
	//void alpacaDeviceShouldFailToInitializeWithInvalidDevice(); // probably useless test
};

#endif // TESTTELESCOPECONTROL_ALPACA_HPP
