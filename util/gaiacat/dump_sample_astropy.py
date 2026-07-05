import struct
import argparse
import os
from astropy.coordinates import SkyCoord
import astropy.units as u
from astropy_healpix import HEALPix

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--cat', required=True, help='path to level N .cat file')
    ap.add_argument('--level', type=int, required=True, help='geodesic level')
    ap.add_argument('--sample', type=int, default=10000, help='number of samples')
    ap.add_argument('--seed', type=int, default=42)
    ap.add_argument('--out', required=True, help='output CSV')
    args = ap.parse_args()

    hp = HEALPix(nside=4096, order='nested', frame='icrs')

    # Read zone count table header. Layout (from ZoneArray.cpp read()):
    # 2 bytes mag_min, 2 bytes mag_range, 2 bytes mag_steps, 4 bytes level
    # Actually it is: Int16 mag_min, Int16 mag_range, Uint32 nb_levels? Let's just read whole header.
    # The catalog starts with: magic? No. Simpler: use loadCat logic values.
    # However easiest is to reuse Stellarium: let testGaiaSearch dump a CSV of sampled gaiaId,ra,dec.
    # But here we implement direct reader.

    with open(args.cat, 'rb') as f:
        data = f.read()

    # Header size for level N is: file starts with zone count table for nr_of_zones zones.
    # For non-compact: header = 28 + 4*nr_of_zones bytes (see ZoneArray.cpp).
    nr_of_zones = 20 * (1 << (2 * args.level)) + 1
    header_size = 28 + 4 * nr_of_zones
    counts = struct.unpack_from('<' + 'I' * nr_of_zones, data, 28)
    total_stars = sum(counts)
    print(f'level={args.level} zones={nr_of_zones} total_stars={total_stars} header_size={header_size}')

    star_size = 32  # Star2
    # data stars start at header_size
    import numpy as np
    rng = np.random.default_rng(args.seed)
    stride = max(1, total_stars // args.sample)
    indices = np.arange(0, total_stars, stride, dtype=np.uint64)[:args.sample]

    # Build cumulative offsets to locate star by global index
    cum = [0]
    for c in counts:
        cum.append(cum[-1] + c)

    with open(args.out, 'w', encoding='utf-8') as outf:
        outf.write('gaiaId,ra,dec,hp_stel,hp_astropy\n')
        for gi in indices:
            # find zone
            z = 0
            while z + 1 < len(cum) and cum[z + 1] <= gi:
                z += 1
            local = gi - cum[z]
            off = header_size + (cum[z] * star_size) + (local * star_size)
            rec = data[off:off + star_size]
            gaia_id, x0, x1 = struct.unpack_from('<qii', rec, 0)
            import math
            ra = (x0 * 1e-3 / 3600.) * (math.pi / 180.0)
            dec = (x1 * 1e-3 / 3600.) * (math.pi / 180.0)
            hp_enc = gaia_id // 34359738368
            c = SkyCoord(ra=ra * u.rad, dec=dec * u.rad, frame='icrs')
            hp_ap = int(hp.lonlat_to_healpix(c.spherical.lon, c.spherical.lat))
            outf.write(f'{gaia_id},{ra},{dec},{hp_enc},{hp_ap}\n')

if __name__ == '__main__':
    main()
