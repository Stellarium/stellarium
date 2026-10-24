// Photometric conversions: G→V and BP-RP→B-V
// Formulas match Cartes du Ciel (cu_catalog.pas:6500-6540)

#include "convert.hpp"
#include <cmath>
#include <algorithm>

double g_to_v(double g, double c)
{
	if (std::isnan(c))
		return g;
	return g + 0.02704 - 0.01424*c + 0.2156*c*c - 0.01426*c*c*c;
}

double bp_rp_to_bv(double c)
{
	// CdC sets B-V=0 when BP/RP is unavailable
	if (std::isnan(c))
		return 0.0;

	// Degradation: when BP-RP is outside the valid polynomial range
	if (c < -0.3 || c > 3.0) {
		return 0.5 * c;
	}

	double bt_vt = -0.006482 + 0.7865*c - 0.3631*c*c + 0.93192*c*c*c - 0.4843*c*c*c*c + 0.06814*c*c*c*c*c;

	double bv;
	if (bt_vt < 0.5) {
		bv = bt_vt - 0.006 - 0.1069*bt_vt + 0.1459*bt_vt*bt_vt;
	} else if (bt_vt < 2.0) {
		bv = bt_vt - 0.007813*bt_vt - 0.1489*bt_vt*bt_vt + 0.03384*bt_vt*bt_vt*bt_vt;
	} else {
		bv = 0.850 * bt_vt;
	}
	return bv;
}