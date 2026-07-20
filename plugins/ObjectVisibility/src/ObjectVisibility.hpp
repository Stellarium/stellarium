/*
 * Object Visibility plug-in for Stellarium
 *
 * Copyright (C) 2026 Atque
 *
 * Concept inspired by:
 *   Astro-Geo-GIS, "The 49 brightest stars in the night sky – when and
 *   where can we see them?", https://astro-geo-gis.com/
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

#ifndef OBJECTVISIBILITY_HPP
#define OBJECTVISIBILITY_HPP

#include <QObject>
#include "StelModule.hpp"
#include "StelFader.hpp"

class QSettings;
class StelButton;
class ObjectVisibilityDialog;

/*! @defgroup objectVisibility Object Visibility Plug-in
@{
A plugin to visualise where on Earth a given star (or DSO) can be seen,
based on the simple geometric criteria described in the Astro-Geo-GIS
article "The 49 brightest stars in the night sky – when and where can
we see them?".

Given a star with declination @f$\delta@f$ at the current epoch
(precession + proper motion taken into account):

 - **Limit of visibility:**  latitudes outside [@f$\delta-90°@f$,
   @f$\delta+90°@f$] never see the star above the horizon.
 - **"Good visibility" limit (default +5°):**  latitudes within
   [@f$\delta-(90°-h)@f$, @f$\delta+(90°-h)@f$] see the star reach at
   least altitude @f$h@f$ at upper culmination.
 - **Passes zenith:**  at latitude @f$\delta@f$ the star passes through
   the zenith.
 - **Circumpolar limit, north / south hemisphere:**  at latitudes
   @f$\geq 90°-\delta@f$ (resp. @f$\leq -90°-\delta@f$) the star never
   sets.

The plugin does **not** handle planets, the Sun, the Moon, asteroids,
comets or artificial satellites: their motion across the sky is too
fast for a single static snapshot to be useful.

The Twilight limits tab is Earth-only. It draws the equator, tropics,
polar circles, and solstice latitudes where the Sun's lowest or highest
altitude is -6°, -12°, or -18°. These latitudes are computed from
Earth's obliquity of date, so they remain valid for historical and
future epochs supported by Stellarium.
@}
*/

//! Main class for the Object Visibility plug-in.
//! @ingroup objectVisibility
class ObjectVisibility : public StelModule
{
	Q_OBJECT
	// Configurable "good visibility" altitude limit, in degrees.
	// Stored in config.ini; default 5°.
	Q_PROPERTY(int goodVisibilityLimit
	           READ getGoodVisibilityLimit
	           WRITE setGoodVisibilityLimit
	           NOTIFY goodVisibilityLimitChanged)

public:
	ObjectVisibility();
	~ObjectVisibility() override;

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	void init() override;
	void update(double deltaTime) override { Q_UNUSED(deltaTime); }
	// We do not draw anything on the sky.  All drawing happens inside our
	// own widget.  Keeping draw() empty is the cheapest possible
	// implementation, which is important for keeping FPS high.
	void draw(class StelCore* core) override { Q_UNUSED(core); }
	double getCallOrder(StelModuleActionName actionName) const override;
	bool configureGui(bool show=true) override;
	///////////////////////////////////////////////////////////////////////////

	//! Restore default settings to config.ini and reload them.
	void restoreDefaultSettings();
	//! Load settings from config.ini ("ObjectVisibility/..." section).
	void loadSettings();

	int getGoodVisibilityLimit() const { return goodVisibilityLimit; }
	bool getPlaceLabelsVisible() const { return placeLabelsVisible; }
	int getPlaceLabelsMinimumPopulation() const { return placeLabelsMinimumPopulation; }
	bool getPlaceLabelsNearLinesOnly() const { return placeLabelsNearLinesOnly; }
	bool getSyncMaps() const { return syncMaps; }

signals:
	void goodVisibilityLimitChanged(int degrees);

public slots:
	void setGoodVisibilityLimit(int degrees);
	void setPlaceLabelsVisible(bool visible);
	void setPlaceLabelsMinimumPopulation(int population);
	void setPlaceLabelsNearLinesOnly(bool nearLinesOnly);
	void setSyncMaps(bool enabled);

private:
	int goodVisibilityLimit;          //!< degrees, range [1, 89]
	bool placeLabelsVisible;
	int placeLabelsMinimumPopulation;
	bool placeLabelsNearLinesOnly;
	bool syncMaps;
	QSettings* conf;

#ifndef NO_GUI
	ObjectVisibilityDialog* configDialog;
	StelButton* toolbarButton;
#endif
};



#include "StelPluginInterface.hpp"

//! Plug-in interface used by Qt's plug-in system.
class ObjectVisibilityStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID StelPluginInterface_iid)
	Q_INTERFACES(StelPluginInterface)
public:
	StelModule* getStelModule() const override;
	StelPluginInfo getPluginInfo() const override;
};

#endif // OBJECTVISIBILITY_HPP
