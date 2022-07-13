/*
 * Copyright (C) 2022 Alexander Wolf
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

#ifndef SPECIFICTIMEMGR_HPP
#define SPECIFICTIMEMGR_HPP

#include "StelObjectModule.hpp"
#include "StelObject.hpp"

#include <QList>

class StelPainter;
class QSettings;

class SpecificTimeMgr : public StelModule
{
	Q_OBJECT

public:
	SpecificTimeMgr();
	virtual ~SpecificTimeMgr() Q_DECL_OVERRIDE;

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	virtual void init() Q_DECL_OVERRIDE;
	virtual void deinit() Q_DECL_OVERRIDE;
	virtual void draw(StelCore* core) Q_DECL_OVERRIDE;
	virtual double getCallOrder(StelModuleActionName actionName) const Q_DECL_OVERRIDE;

private:
	QSettings* conf;

};

#endif /* SPECIFICTIMEMGR_HPP */
