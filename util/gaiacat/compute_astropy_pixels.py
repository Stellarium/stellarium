import argparse
import math
from astropy.coordinates import SkyCoord
import astropy.units as u
from astropy_healpix import HEALPix

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--in', dest='infile', required=True, help='CSV with gaiaId,ra,dec (ra/dec in rad)')
    ap.add_argument('--out', required=True, help='output CSV with gaiaId,ra,dec,hp_stel,hp_astropy')
    args = ap.parse_args()

    hp = HEALPix(nside=4096, order='nested', frame='icrs')
    with open(args.infile, 'r', encoding='utf-8') as inf, \
         open(args.out, 'w', encoding='utf-8') as outf:
        header = inf.readline().strip()
        outf.write('gaiaId,ra,dec,hp_stel,hp_astropy\n')
        for line in inf:
            line = line.strip()
            if not line:
                continue
            parts = line.split(',')
            gid = int(parts[0])
            ra = float(parts[1])
            dec = float(parts[2])
            hp_enc = gid // 34359738368
            c = SkyCoord(ra=ra * u.rad, dec=dec * u.rad, frame='icrs')
            hp_ap = int(hp.lonlat_to_healpix(c.spherical.lon, c.spherical.lat))
            outf.write(f'{gid},{ra},{dec},{hp_enc},{hp_ap}\n')

if __name__ == '__main__':
    main()
