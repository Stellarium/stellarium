/*
 * Stellarium
 * Copyright (C) 2018-2022 Ruslan Kabatsayev <b7.10110111@gmail.com>
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

#ifndef DITHERING_HPP
#define DITHERING_HPP

#include "VecMath.hpp"
#include "StelOpenGL.hpp"
#include "StelPainter.hpp"

namespace ForTextureMgr
{
GLuint makeDitherPatternTexture(QOpenGLFunctions& gl);
}

enum class DitheringMode
{
	Disabled,    //!< Dithering disabled, will leave the infamous color bands
	Color565,    //!< 16-bit color (AKA High color) with R5_G6_B5 layout
	Color666,    //!< TN+film typical color depth in TrueColor mode
	Color888,    //!< 24-bit color (AKA True color)
	Color101010, //!< 30-bit color (AKA Deep color)
};
#if QT_VERSION < QT_VERSION_CHECK(5,14,0)
// A similar pair of methods but templated for arbitrary enum first appears in Qt 5.14
inline QDataStream& operator>>(QDataStream& s, DitheringMode& m)
{
	return s >> reinterpret_cast<std::underlying_type<DitheringMode>::type&>(m);
}
inline QDataStream& operator<<(QDataStream &s, const DitheringMode &m)
{
	return s << static_cast<typename std::underlying_type<DitheringMode>::type>(m);
}
#endif

Vec3f calcRGBMaxValue(DitheringMode mode);

QString makeDitheringShader();

Q_DECLARE_METATYPE(DitheringMode)

#endif
