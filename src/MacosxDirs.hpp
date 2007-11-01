/*
 * Stellarium
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef MACOSXDIRS_HPP_
#define MACOSXDIRS_HPP_

#include <string>

using namespace std;

/**
 * Functions to use MacOSX's Carbon Framework to retrieve and make
 * usable as string's the directories of the application and its
 * resource directories.
 */

class MacosxDirs
{
public:
	//! Return the full path to the ".app" directory that corresponds to this macosx application.
	//! @return the full path.
	static QString getApplicationDirectory();


	//! Return the full path of the Resources directory inside this macosx application.
	//! @return the full path.
	static QString getApplicationResourcesDirectory();
};

#endif

