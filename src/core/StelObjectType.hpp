/*
 * Stellarium
 * Copyright (C) 2007 Fabien Chereau
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

#ifndef STELOBJECTTYPE_HPP
#define STELOBJECTTYPE_HPP

#include <QSharedPointer>

//! Special version of QSharedPointer which by default doesn't delete the referenced pointer when
//! the reference count reaches 0.
template <class T> class QSharedPointerNoDelete : public QSharedPointer<T>
{
public:
	QSharedPointerNoDelete() {;}
	QSharedPointerNoDelete(T *ptr) : QSharedPointer<T>(ptr, QSharedPointerNoDelete::noDelete) {;}
	QSharedPointerNoDelete(T *ptr, bool own) : QSharedPointer<T>(ptr) {Q_UNUSED(own); Q_ASSERT(own==true);}
	QSharedPointerNoDelete(const QSharedPointer<T>& ptr) : QSharedPointer<T>(ptr) {;}
	static void noDelete(T *ptr) {Q_UNUSED(ptr);}
};

//! @file StelObjectType.hpp
//! Define the StelObjectP type.

class StelObject;

//! @typedef StelObjectP
//! Intrusive pointer used to manage StelObject with smart pointers
typedef QSharedPointerNoDelete<StelObject> StelObjectP;

Q_DECLARE_METATYPE(StelObjectP)

#endif // STELOBJECTTYPE_HPP
