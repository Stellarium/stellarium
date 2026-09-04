// Photometric conversion declarations
#pragma once

// Convert Gaia G magnitude to Johnson V magnitude
// g: G-band magnitude
// c: BP-RP color index (NaN for missing BP/RP — returns g directly)
// Returns V magnitude
double g_to_v(double g, double c);

// Convert BP-RP to Johnson B-V via two-step polynomial
// c: BP-RP color index (NaN for missing data — returns 0.5)
// Returns B-V index, clamped to [0.0, 5.35]
double bp_rp_to_bv(double c);
