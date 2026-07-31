#!/usr/bin/env python3
"""
ERFA zone helper for hip2cat: performs the +/-210 kyr past/future global-zone
analysis using astropy, exactly matching the official Stellarium pipeline.

Input:  binary file (arg1) 鈥?uint32 n_stars, then n_stars x 7 doubles:
         ra(deg), dec(deg), pmra(mas/yr), pmdec(mas/yr), plx(mas), rv(km/s), plxe(mas)

Output: binary file (arg2) 鈥?n_stars x uint8  global_zone_flag (0/1).
"""

import sys
import struct
import numpy as np
import astropy.units as u
from astropy.coordinates import SkyCoord
from astropy.time import Time

_level = int(sys.argv[3]) if len(sys.argv) > 3 else 0

sys.path.insert(0, r"C:\Users\13308\CLionProjects\stellarium_star_catalogs")
from py.geodesic import GeodesicGrid

with open(sys.argv[1], 'rb') as f:
    n = struct.unpack('<I', f.read(4))[0]
    data = np.zeros((n, 7), dtype=np.float64)
    for i in range(n):
        data[i] = struct.unpack('<7d', f.read(56))

ra, dec = data[:, 0], data[:, 1]
pmra, pmdec = data[:, 2], data[:, 3]
plx, rv = data[:, 4], data[:, 5]
plxe = data[:, 6]

# ---- good astrometry filter (matches henrysky: plx/plx_err > 5) ----
good = np.zeros(n, dtype=bool)
for i in range(n):
    has = (plx[i] > 0.0) or (plxe[i] > 0.0)
    if has:
        se = plxe[i] if plxe[i] > 1e-12 else 1e-12
        good[i] = (plx[i] / se) > 5.0

# distance fallback for zero/NaN parallax: np.inf, matching henrysky's
# 1/parallax division. A finite cap (e.g. 1e12 pc) makes ERFA override the
# distance and kills the global-zone crossing detection for plx=0 stars.
with np.errstate(divide="ignore"):
    dist_fallback = np.inf

# ---- initial zone ----
g = GeodesicGrid(level=_level)
c_now = SkyCoord(ra=ra * u.deg, dec=dec * u.deg,
                 distance=(np.where(plx > 1e-12, 1000.0 / plx, dist_fallback)) * u.pc,
                 pm_ra_cosdec=pmra * u.mas / u.yr,
                 pm_dec=pmdec * u.mas / u.yr,
                 radial_velocity=np.nan_to_num(rv, nan=0) * u.km / u.s,
                 obstime=Time(2016.0, format="jyear"), frame="icrs")
zone_past = np.asarray(g.search_zone(c_now.cartesian.xyz.T), dtype=np.int32)
zone_future = zone_past.copy()

counter_past = np.zeros(n, dtype=np.int32)
counter_future = np.zeros(n, dtype=np.int32)
max_vmag_diff = np.zeros(n)  # init 0: any negative = brighter

# ---- propagation steps ----
for iyr in range(10000, 210000, 10000):
    # past  (backward: iyr -> 0)
    cp = SkyCoord(ra=ra * u.deg, dec=dec * u.deg,
                  distance=(np.where(plx > 1e-12, 1000.0 / plx, dist_fallback)) * u.pc,
                  pm_ra_cosdec=pmra * u.mas / u.yr,
                  pm_dec=pmdec * u.mas / u.yr,
                  radial_velocity=np.nan_to_num(rv, nan=0) * u.km / u.s,
                  obstime=Time(iyr, format="jyear"), frame="icrs")
    cp = cp.apply_space_motion(Time(0.0, format="jyear"))
    tz_past = np.asarray(g.search_zone(cp.cartesian.xyz.T), dtype=np.int32)
    counter_past += (tz_past != zone_past).astype(np.int32)
    zone_past = tz_past
    if good.any():
        dp = cp.distance.to(u.pc).value
        diff = 5.0 * np.log10(np.where(plx > 1e-12, dp * plx / 1000.0, 1.0))
        max_vmag_diff = np.minimum(max_vmag_diff, diff)  # negative = brighter

    # future (forward: 0 -> iyr)
    cf = SkyCoord(ra=ra * u.deg, dec=dec * u.deg,
                  distance=(np.where(plx > 1e-12, 1000.0 / plx, dist_fallback)) * u.pc,
                  pm_ra_cosdec=pmra * u.mas / u.yr,
                  pm_dec=pmdec * u.mas / u.yr,
                  radial_velocity=np.nan_to_num(rv, nan=0) * u.km / u.s,
                  obstime=Time(0.0, format="jyear"), frame="icrs")
    cf = cf.apply_space_motion(Time(iyr, format="jyear"))
    tz_fut = np.asarray(g.search_zone(cf.cartesian.xyz.T), dtype=np.int32)
    counter_future += (tz_fut != zone_future).astype(np.int32)
    zone_future = tz_fut
    if good.any():
        df = cf.distance.to(u.pc).value
        diff = 5.0 * np.log10(np.where(plx > 1e-12, df * plx / 1000.0, 1.0))
        max_vmag_diff = np.minimum(max_vmag_diff, diff)

# ---- rules (identical to henrysky notebook) ----
max_vmag_diff[~good] = 0.0
global_zone = np.zeros(n, dtype=np.uint8)
if _level == 0:
    mask = (counter_past + counter_future > 0) & good
else:
    mask = (counter_past > 1) | (counter_future > 1) | (max_vmag_diff < -0.3)
global_zone[mask] = 1

with open(sys.argv[2], 'wb') as f:
    f.write(global_zone.tobytes())

print(f"  ERFA lv{_level}: {int(global_zone.sum())} global of {n}")
