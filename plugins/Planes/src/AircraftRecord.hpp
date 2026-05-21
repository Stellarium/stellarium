#ifndef PLANES_AIRCRAFTRECORD_HPP
#define PLANES_AIRCRAFTRECORD_HPP

#include <QString>

struct AircraftRecord
{
	QString icao24;
	QString callsign;
	QString aircraftType;
	double latitude = 0.0;
	double longitude = 0.0;
	double altitudeMeters = 0.0;
	double groundSpeedMs = 0.0;
	double trackDegrees = 0.0;
	double verticalRateMs = 0.0;
	double snapshotJd = 0.0;
};

#endif
