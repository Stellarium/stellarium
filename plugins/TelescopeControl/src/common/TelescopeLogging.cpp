/*
 * Copyright (C) 2026 Ruslan Kabatsayev <b7.10110111@gmail.com>
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

#include "TelescopeLogging.hpp"

Q_LOGGING_CATEGORY(Telescopes, "stel.Telescopes", QtInfoMsg)

QDebug operator<<(QDebug debug, TelescopeLogPrefix prefix)
{
	if (prefix.slot > 0)
	{
		const bool spaces = debug.autoInsertSpaces();
		debug.nospace() << '[' << prefix.slot << "] ";
		debug.setAutoInsertSpaces(spaces);
	}
	return debug;
}
