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


//! @class FlightMgr
//! Manages updating, drawing and searching for Flight objects.
//! Accesses a FlightDataSource and gets commands from Planes
class FlightMgr : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool labelsVisible READ isLabelsVisible WRITE setLabelsVisible)
    Q_PROPERTY(bool useInterp READ isInterpEnabled WRITE setInterpEnabled)
public:
    //! Constructor
    explicit FlightMgr(QObject *parent = 0);
    ~FlightMgr();

    //! Draws all the flights
    void draw(StelCore *core);

    //! Draws the selection rectangle
    void drawPointer(StelCore *core, StelPainter &painter);

    //! Update all Flight objects and the dataSource, if available
    void update(double deltaTime);


    //! Search around a point in space
    QList<StelObjectP> searchAround(const Vec3d &v, double limitFov, const StelCore *core) const;

    //! Search by translated name.
    //! Since Flight names aren't translated, just calls searchByName()
    StelObjectP searchByNameI18n(const QString &nameI18n) const;

    //! Search by name. Attempts to match callsign
    StelObjectP searchByName(const QString &name) const;

    //! Find objects with partially matching names (translated).
    //! Calls listMatchingObjects() as names aren't translated.
    QStringList listMatchingObjectsI18n(const QString &objPrefix, int maxNbItem, bool useStartOfWords) const;

    //! Find objects with partially matching names.
    //! Compares to callsign and mode s hex address
    QStringList listMatchingObjects(const QString &objPrefix, int maxNbItem, bool useStartOfWords) const;

    //! Returns a list of all objects.
    QStringList listAllObjects(bool inEnglish) const;

    //! Change the data source.
    //! @param source a pointer to the data source to use.
    void setDataSource(FlightDataSource *source);

    //! Check whether labels are drawn
    bool isLabelsVisible() const
    {
        return labelsVisible;
    }

    //! Check whether interp is enabled
    bool isInterpEnabled() const
    {
        return ADSBData::useInterp;
    }

    //! Get the path drawing mode
    Flight::PathDrawMode getPathDrawMode() const
    {
        return pathDrawMode;
    }

    //! Get the path colouring mode
    Flight::PathColour getPathColourMode() const
    {
        return Flight::getPathColourMode();
    }

signals:

public slots:
    //! Set the brightness of the renderer, used to fade in and out
    void setBrightness(double b)
    {
        displayBrightness = b;
    }

    //! User clicked on an object, check if the object is a Flight.
    //! If it is, draw selection rectagle and mark as selected.
    //! If not, mark last selected Flight as no longer selected.
    void selectedObjectChanged();

    //! Change the path colouring mode
    void setPathColourMode(Flight::PathColour mode)
    {
        Flight::setPathColourMode(mode);
    }

    //! Change the path drawing mode
    void setPathDrawMode(Flight::PathDrawMode mode)
    {
        pathDrawMode = mode;
    }

    //! Turn label drawing on or off
    void setLabelsVisible(bool visible)
    {
        labelsVisible = visible;
    }

    //! Turn interpolation on or off
    void setInterpEnabled(bool interp);

private:
    bool labelsVisible; //!< are labels shown
    Flight::PathDrawMode pathDrawMode; //!< Path drawing mode
    StelTextureSP texPointer; //!< Selection rectangle
    FlightDataSource *dataSource; //!< current data source
    QSharedPointer<Planet> earth; //!< Reference to planet earth, to check if we are on earth
    double displayBrightness; //!< brightness for rendering / fading
    FlightP lastSelectedObject; //!< the last selected Flight

};

#endif // FLIGHTMGR_HPP
