/*
 * Copyright (C) 2013 Felix Zeltner
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

#ifndef FLIGHTMGR_HPP
#define FLIGHTMGR_HPP

#include <QObject>
#include "Planet.hpp"
#include "StelTexture.hpp"
#include "Flight.hpp"
#include "FlightDataSource.hpp"


class FlightMgr : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool labelsVisible READ isLabelsVisible WRITE setLabelsVisible)
    Q_PROPERTY(bool useInterp READ isInterpEnabled WRITE setInterpEnabled)
public:
    explicit FlightMgr(QObject *parent = 0);
    ~FlightMgr();
    void draw(StelCore *core);
    void drawPointer(StelCore *core, StelPainter &painter);
    void update(double deltaTime);


    QList<StelObjectP> searchAround(const Vec3d &v, double limitFov, const StelCore *core) const;

    StelObjectP searchByNameI18n(const QString &nameI18n) const;

    StelObjectP searchByName(const QString &name) const;

    QStringList listMatchingObjectsI18n(const QString &objPrefix, int maxNbItem, bool useStartOfWords) const;

    QStringList listMatchingObjects(const QString &objPrefix, int maxNbItem, bool useStartOfWords) const;

    QStringList listAllObjects(bool inEnglish) const;

    void setDataSource(FlightDataSource *source);

    bool isLabelsVisible() const
    {
        return labelsVisible;
    }
    bool isInterpEnabled() const
    {
        return ADSBData::useInterp;
    }
    Flight::PathDrawMode getPathDrawMode() const
    {
        return pathDrawMode;
    }
    Flight::PathColour getPathColourMode() const
    {
        return Flight::getPathColourMode();
    }

signals:

public slots:
    void setBrightness(double b)
    {
        displayBrightness = b;
    }
    void selectedObjectChanged();
    void setPathColourMode(Flight::PathColour mode)
    {
        Flight::setPathColourMode(mode);
    }
    void setPathDrawMode(Flight::PathDrawMode mode)
    {
        pathDrawMode = mode;
    }
    void setLabelsVisible(bool visible)
    {
        labelsVisible = visible;
    }
    void setInterpEnabled(bool interp);

private:
    bool labelsVisible;
    Flight::PathDrawMode pathDrawMode;
    StelTextureSP texPointer; //<! Selection rectangle
    FlightDataSource *dataSource;
    QSharedPointer<Planet> earth;
    double displayBrightness;
    FlightP lastSelectedObject;

};

#endif // FLIGHTMGR_HPP
