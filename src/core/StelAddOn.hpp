/*
 * Stellarium
 * Copyright (C) 2014 Marcos Cardinot
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

#ifndef _STELADDON_HPP_
#define _STELADDON_HPP_

#include <QDateTime>
#include <QSqlDatabase>

class StelAddOn
{
public:
	StelAddOn();

	void setLastUpdate(qint64 time);
	QString getLatUpdate()
	{
		return QDateTime::fromMSecsSinceEpoch(m_iLastUpdate)
				.toString("dd MMM yyyy - hh:mm:ss");
	}
	qint64 getLastUpdate()
	{
		return m_iLastUpdate;
	}

private:
	QSqlDatabase m_db;
	QString m_sAddonPath;
	qint64 m_iLastUpdate;

	bool createAddonTables();
	bool createTableLicense();
	bool createTableAuthor();
};

#endif // _STELADDON_HPP_
