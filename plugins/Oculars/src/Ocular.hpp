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

#include <QDebug>
#include <QExplicitlySharedDataPointer>
#include <QObject>
#include <QSettings>
#include <QString>
#include <QtGlobal>

#include "OcularsConfig.hpp"

class Telescope;
class Lens;

//! @ingroup oculars
class Ocular : public QObject
{
   Q_OBJECT
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
   Q_DISABLE_COPY_MOVE(Ocular)
#endif
   Q_PROPERTY(bool binoculars READ isBinoculars WRITE setBinoculars)
   Q_PROPERTY(bool permanentCrosshair READ hasPermanentCrosshair WRITE setPermanentCrosshair)
   Q_PROPERTY(double apparentFOV READ apparentFOV WRITE setApparentFOV)
   Q_PROPERTY(double effectiveFocalLength READ effectiveFocalLength WRITE setEffectiveFocalLength)
   Q_PROPERTY(double fieldStop READ fieldStop WRITE setFieldStop)
   Q_PROPERTY(QString name READ name WRITE setName)
   Q_PROPERTY(QString reticlePath READ reticlePath WRITE setReticlePath)
public:
   /// Creates a new instance of Ocular.
   //! The newly created instance will have values initialized to represent a usable ocular model.
   Q_INVOKABLE explicit Ocular(QObject * parent = nullptr);
   ~Ocular() override = default;

   void        initFromSettings(const QSettings * theSettings, int ocularIndex);
   void        writeToSettings(QSettings * settings, int index) const;

   auto        isBinoculars() const -> bool;
   void        setBinoculars(bool flag);
   auto        hasPermanentCrosshair() const -> bool;
   void        setPermanentCrosshair(bool flag);
   auto        apparentFOV() const -> double;
   void        setApparentFOV(double fov);
   auto        effectiveFocalLength() const -> double;
   void        setEffectiveFocalLength(double fl);
   auto        fieldStop() const -> double;
   void        setFieldStop(double fs);
   auto        name() const -> QString;
   void        setName(QString aName);
   auto        reticlePath() const -> QString;
   void        setReticlePath(QString path);

   auto        actualFOV(const Telescope * telescope, const Lens * lens) const -> double;
   auto        magnification(const Telescope * telescope, const Lens * lens) const -> double;
   static auto propertyMap() -> QMap<int, QString>;

private:
   double  m_apparentFOV{ OcularDefaultFOV }; // millimeters
   bool    m_binoculars{ false };
   double  m_effectiveFocalLength{ OcularDefaultFocalLength }; // millimeters
   double  m_fieldStop{ 0.0 };                                 // millimeters
   QString m_name{ "New Ocular" };
   bool    m_permanentCrosshair{ false };
   QString m_reticlePath;
};

auto operator<<(QDebug debug, const Ocular & ocular) -> QDebug;
auto operator<<(QDebug debug, const Ocular * ocular) -> QDebug;

Q_DECLARE_METATYPE(Ocular *);
