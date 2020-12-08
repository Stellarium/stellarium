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

#ifndef TELESCOPECLIENTINDI_HPP
#define TELESCOPECLIENTINDI_HPP

#include "TelescopeClient.hpp"

#include "INDIConnection.hpp"

class TelescopeClientINDI : public TelescopeClient
{
	Q_OBJECT

public:
	TelescopeClientINDI(const QString &name = "INDI", const QString& params = QString());
	~TelescopeClientINDI();

	Vec3d getJ2000EquatorialPos(const StelCore *core) const override;
	void move(double angle, double speed) override;
	void telescopeGoto(const Vec3d &j2000Pos, StelObjectP selectObject) override;
	void telescopeSync(const Vec3d &j2000Pos, StelObjectP selectObject) override;
	bool isConnected() const override;
	bool hasKnownPosition() const override;
	QWidget *createControlWidget(QSharedPointer<TelescopeClient> telescope, QWidget* parent = Q_NULLPTR) const override;

private:
	int toINDISpeed(double speed) const;

	INDIConnection mConnection;
	QString mDevice;
};

#endif // TELESCOPECLIENTINDI_HPP
