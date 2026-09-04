// Geodesic grid zone computation — self-contained, matches StelGeodesicGrid
#pragma once
#include <cstdint>
#include <cmath>

// Compute the geodesic zone number for a 3D unit vector at the given level
// x, y, z: unit vector components (normalized direction on sphere)
// level:   geodesic grid level (0-10)
// Returns: zone index [0, nr_of_zones(level))
int zone_number(double x, double y, double z, int level);

// Number of zones at the given level: 20*4^level + 1
inline int nr_of_zones(int level) { return 20 * (1 << (level * 2)) + 1; }
