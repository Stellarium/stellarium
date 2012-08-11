/*
 * Stellarium
 * Copyright (C) 2012 Ferdinand Majerech
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

#include "StelTextureBackend.hpp"


QString textureStatusName(const TextureStatus status)
{
	switch(status)
	{
		case TextureStatus_Uninitialized: return "TextureStatus_Uninitialized"; break;
		case TextureStatus_Loaded:        return "TextureStatus_Loaded"; break;
		case TextureStatus_Loading:       return "TextureStatus_Loading"; break;
		case TextureStatus_Error:         return "TextureStatus_Error"; break;
		default: Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown texture status");
	}
	// Avoid compiler warnings.
	return QString();
}


