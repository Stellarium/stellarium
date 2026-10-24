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

#ifndef TELESCOPELOGGING_HPP
#define TELESCOPELOGGING_HPP

#include <QDebug>
#include <QLoggingCategory>

//! @note You can finetune the amount of TelescopeControl related messages in the logfile by
//! configuring the logging category stel.Telescopes. For this, e.g. set environment variable
//! QT_LOGGING_RULES="*.debug=false;stel.Telescopes.debug=true;"
//! By default, only Info and more severe messages are displayed.
Q_DECLARE_LOGGING_CATEGORY(Telescopes)

//! Slot identification for telescope diagnostics. A nonpositive slot omits the prefix.
struct TelescopeLogPrefix
{
	explicit TelescopeLogPrefix(int slot)
		: slot(slot)
	{
	}
	int slot;
};

QDebug operator<<(QDebug debug, TelescopeLogPrefix prefix);

#endif // TELESCOPELOGGING_HPP
