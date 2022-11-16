/*
 * Stellarium
 * Copyright (C) 2022 Georg Zotti
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

#ifndef STELMAIN_PCH_HPP
#define STELMAIN_PCH_HPP

// Put most-used header files of the project here.
// Obviously, ignore clangd warnings about unused headers!

// The following list was found with a tool taken from https://github.com/KDAB/kdabtv/tree/master/Qt-Widgets-and-more/compileTime
// I list headers counted from a plugin-free build. The headers here are used 40 or more times, sorted from over 200 uses downward.
// It seems that inclusion of some headers is actually bad for build time, these are commented away again. More teting or other compiler combinations may have slightly other results.

// Base time was 284s from first test without PCH
#include "StelApp.hpp"
#include<QDebug>
#include "StelUtils.hpp"
//#include<QString>
#include "StelCore.hpp"
//#include<QObject>
#include "StelTranslator.hpp"
#include "StelModuleMgr.hpp"
#include<QtGlobal>
//#include<memory>
#include<QSettings>
#include<QVariant>
#include<QStringList>
#include<QDir>
#include "StelFileMgr.hpp"
#include "VecMath.hpp"
//#include "StelPainter.hpp"
#include<QFile>
//#include "StelProjector.hpp"
#include<QList>
#include "StelGui.hpp"
//#include<QRegularExpression>
#include "StelObjectMgr.hpp"
#include "StelDialog.hpp"
// 166s so far with this set.
//#include "StelModule.hpp"
//#include<QMap>
//#include "StelLocaleMgr.hpp"
//#include "StelObject.hpp"
//#include "Planet.hpp"
//#include<cmath>
//#include<QDateTime>
//#include "SolarSystem.hpp"
//#include<QTimer>
//#include<QFont>

#endif // STELMAIN_PCH_HPP
