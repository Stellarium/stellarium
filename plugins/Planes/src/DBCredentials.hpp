/*
 * Copyright (C) 2013 Felix Zeltner
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

#ifndef DBCREDENTIALS_HPP
#define DBCREDENTIALS_HPP

#include <QString>
//! @struct DBCredentials
//! Struct that holds credentials to connect to a database
typedef struct db_creds_s
{
	QString type; //!< Database driver type
	QString host; //!< hostname
	int port; //!< port
	QString user; //!< username
	QString pass; //!< password
	QString name; //!< Database name or filename if the driver is SQLite
} DBCredentials;

#endif // DBCREDENTIALS_HPP
