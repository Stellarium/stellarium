/*
 * Copyright (C) 2009 Timothy Reaves
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

#pragma once

#include <QObject>
#include <QSettings>
#include <QString>
#include <QtGlobal>

#include "OcularsConfig.hpp"

class TelescopeData;

//! @ingroup oculars
class Telescope : public QObject
{
   Q_OBJECT
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
   Q_DISABLE_COPY_MOVE(Telescope)
#endif
   Q_PROPERTY(QString name READ name WRITE setName)
   Q_PROPERTY(double diameter READ diameter WRITE setDiameter)
   Q_PROPERTY(double focalLength READ focalLength WRITE setFocalLength)
   Q_PROPERTY(bool hFlipped READ isHFlipped WRITE setHFlipped)
   Q_PROPERTY(bool vFlipped READ isVFlipped WRITE setVFlipped)
   Q_PROPERTY(bool equatorial READ isEquatorial WRITE setEquatorial)
public:
   /// Creates a new instance of Telescope.
   //! The newly created instance will have values initialized to represent a usable telescope model.
   explicit Telescope(QObject * parent = nullptr);
   ~Telescope() override = default;

   void        initFromSettings(QSettings * theSettings, int telescopeIndex);
   void        writeToSettings(QSettings * settings, int index) const;

   auto        diameter() const -> double;
   void        setDiameter(double theValue);
   auto        focalLength() const -> double;
   void        setFocalLength(double theValue);
   auto        name() const -> QString;
   void        setName(QString theValue);
   auto        isHFlipped() const -> bool;
   void        setHFlipped(bool flipped);
   auto        isVFlipped() const -> bool;
   void        setVFlipped(bool flipped);
   auto        isEquatorial() const -> bool;
   void        setEquatorial(bool eq);
   static auto propertyMap() -> QMap<int, QString>;

private:
   double  m_diameter{ TelescopeDefaultDiameter }; // millimeters
   bool    m_equatorial{ true };
   bool    m_flippedHorizontally{ true };
   bool    m_flippedVertically{ true };
   double  m_focalLength{ TelescopeDefaultFocalLength }; // millimeters
   QString m_name{ "New Telescope" };
};

auto operator<<(QDebug debug, const Telescope & telescope) -> QDebug;
auto operator<<(QDebug debug, const Telescope * telescope) -> QDebug;

Q_DECLARE_METATYPE(Telescope *);
