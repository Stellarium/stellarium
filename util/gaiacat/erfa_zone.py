#!/usr/bin/env python3
"""
ERFA zone helper for hip2cat: performs the +/-210 kyr past/future global-zone
analysis using astropy (exact same engine as the official Stellarium pipeline).

Input:  binary file (arg1) — uint32 n_stars, then n_stars x 6 doubles:
         ra(deg), dec(deg), pmra(mas/yr), pmdec(mas/yr), plx(mas), rv(km/s)

Output: binary file (arg2) — n_stars x uint8  global_zone_flag (0/1).
"""

import sys, struct, os
import numpy as np
import astropy.units as u
from astropy.coordinates import SkyCoord
from astropy.time import Time

# find stellarium_star_catalogs/py for the GeodesicGrid import
_cur = os.path.dirname(os.path.abspath(__file__))
_star_cat = os.path.join(_cur, "..", "..", "..", "C:", "Users", "13308", "CLionProjects", "stellarium_star_catalogs")
if os.path.isdir(_star_cat): sys.path.insert(0, _star_cat)
# also try a sibling approach: this script lives under <repo>/util/gaiacat/,
# and the stellarium_star_catalogs sits at <repo>/../stellarium_star_catalogs? No.
# Hardcode the known path as a fallback.
sys.path.insert(0, r"C:\Users\13308\CLionProjects\stellarium_star_catalogs")
from py.geodesic import GeodesicGrid, radec2xyz

level = 0
if len(sys.argv) >= 4:
    level = int(sys.argv[3])

with open(sys.argv[1], 'rb') as f:
    n = struct.unpack('<I', f.read(4))[0]
    data = np.zeros((n, 6), dtype=np.float64)
    for i in range(n):
        data[i] = struct.unpack('<6d', f.read(48))

ra, dec = data[:, 0], data[:, 1]
pmra, pmdec = data[:, 2], data[:, 3]
plx, rv = data[:, 4], data[:, 5]

c = SkyCoord(ra=ra * u.deg, dec=dec * u.deg,
             distance=(np.where(plx > 0, 1.0 / plx, 1e12)) * u.kpc,
             pm_ra_cosdec=pmra * u.mas / u.yr,
             pm_dec=pmdec * u.mas / u.yr,
             radial_velocity=np.nan_to_num(rv, nan=0, posinf=0, neginf=0) * u.km / u.s,
             obstime=Time(2016.0, format="jyear"), frame="icrs")

g = GeodesicGrid(level=level)
zone_now = np.asarray(g.search_zone(c.cartesian.xyz.T))
zone_past  = zone_now.copy()
zone_future = zone_now.copy()
good = (plx > 0) & (plx / np.maximum(plx * 0 + 1e-12, np.where(plx > 0, plx, 1e12)) > 5)
counter_past   = np.zeros(n, dtype=np.int32)
counter_future = np.zeros(n, dtype=np.int32)
max_vmag_diff  = np.zeros(n)

for iyr in range(10000, 210000, 10000):
    c_past = SkyCoord(ra=ra * u.deg, dec=dec * u.deg,
                      distance=(np.where(plx > 0, 1.0 / plx, 1e12)) * u.kpc,
                      pm_ra_cosdec=pmra * u.mas / u.yr,
                      pm_dec=pmdec * u.mas / u.yr,
                      radial_velocity=np.nan_to_num(rv, nan=0, posinf=0, neginf=0) * u.km / u.s,
                      obstime=Time(iyr, format="jyear"), frame="icrs")
    c_past = c_past.apply_space_motion(Time(0.0, format="jyear"))
    temp_zone = np.asarray(g.search_zone(c_past.cartesian.xyz.T))
    counter_past += (temp_zone != zone_past).astype(np.int32)
    zone_past = temp_zone

    c_future = SkyCoord(ra=ra * u.deg, dec=dec * u.deg,
                        distance=(np.where(plx > 0, 1.0 / plx, 1e12)) * u.kpc,
                        pm_ra_cosdec=pmra * u.mas / u.yr,
                        pm_dec=pmdec * u.mas / u.yr,
                        radial_velocity=np.nan_to_num(rv, nan=0, posinf=0, neginf=0) * u.km / u.s,
                        obstime=Time(0.0, format="jyear"), frame="icrs")
    c_future = c_future.apply_space_motion(Time(iyr, format="jyear"))
    temp_zone = np.asarray(g.search_zone(c_future.cartesian.xyz.T))
    counter_future += (temp_zone != zone_future).astype(np.int32)
    zone_future = temp_zone

    if good.any():
        zd = (1.0 / np.maximum(c_future.distance.to(u.pc).value, 1e-12)
              * np.where(plx > 0, plx, 1e-12))  # d0/dk ratio
        dk0 = 5.0 * np.log10(c_future.distance.to(u.pc).value / np.where(plx > 0, 1000.0/plx, 1e12))
        max_vmag_diff = np.minimum(max_vmag_diff, dk0)

# rules
global_zone = np.zeros(n, dtype=np.uint8)
if level == 0:
    mask = (counter_past + counter_future > 0) & good
else:
    mask = (counter_past > 1) | (counter_future > 1) | ((max_vmag_diff < -0.3) & good)
global_zone[mask] = 1

with open(sys.argv[2], 'wb') as f:
    f.write(global_zone.tobytes())

print(f"  ERFA lv{level}: {global_zone.sum()} global of {n}")
