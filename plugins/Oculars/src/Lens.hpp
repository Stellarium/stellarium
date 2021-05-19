/*
 * Copyright (C) 2009 Timothy Reaves
 * Copyright (C) 2013 Pawel Stolowski
 * Copyright (C) 2013 Alexander Wolf
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

#include <QDebug>
#include <QExplicitlySharedDataPointer>
#include <QMap>
#include <QObject>
#include <QString>
#include <QtGlobal>

class QSettings;

//! @ingroup oculars
class Lens : public QObject
{
   Q_OBJECT
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
   Q_DISABLE_COPY_MOVE(Lens)
#endif
   Q_PROPERTY(QString name READ name WRITE setName)
   Q_PROPERTY(double multiplier READ multiplier WRITE setMultiplier)

public:
   /// Creates a new instance of Lens.
   //! The newly created instance will have values initialized to represent a usable lens model.
   Q_INVOKABLE explicit Lens(QObject * parent = nullptr);
   ~Lens() override = default;

   void        initFromSettings(QSettings * theSettings, int lensIndex);
   void        writeToSettings(QSettings * settings, int index) const;

   auto        multiplier() const -> double;
   void        setMultiplier(double theValue);
   auto        name() const -> QString;
   void        setName(const QString & theValue);
   static auto propertyMap() -> QMap<int, QString>;

private:
   double  m_multiplier{ 0.0 };
   QString m_name{ "New Lens" };
};

auto operator<<(QDebug debug, const Lens & lens) -> QDebug;
auto operator<<(QDebug debug, const Lens * lens) -> QDebug;

Q_DECLARE_METATYPE(Lens *);
