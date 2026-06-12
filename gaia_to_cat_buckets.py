#!/usr/bin/env python3
"""
Gaia DR3 CSV -> Stellarium .cat converter (bucket-based, low memory).

Three passes per level:
  Pass 1: count stars per zone -> pre-allocate .cat
  Pass 2: scatter stars into bucket files by zone range
  Pass 3: for each bucket, sort by (zone, vmag) -> write .cat

Usage: python gaia_to_cat_buckets.py --csv-dir DIR --out-dir DIR
"""

import subprocess
import struct
import os
import sys
import math
import argparse
import tempfile
import shutil
import io
import gzip
from collections import defaultdict

# ── Configuration ───────────────────────────────────────────────

FILE_MAGIC      = 0x835F040A
CATALOG_EPOCH   = 2457389.0     # J2015.5
STAR3_REC_SIZE  = 16            # bytes

# Final level plan (only 18+)
LEVELS = [
    {"name": "stars_9",  "level": 9,  "mag_lo": 18.0, "mag_hi": 20.0, "buckets": 12},
    {"name": "stars_10", "level": 10, "mag_lo": 20.0, "mag_hi": 22.0, "buckets": 48},
]

# ── Geodesic grid ───────────────────────────────────────────────

PHI  = (1.0 + math.sqrt(5.0)) / 2.0
NORM = math.sqrt(1.0 + PHI * PHI)
_icosahedron_a = PHI / NORM
_icosahedron_b = 1.0 / NORM

ICOSAHEDRON_VERTICES = [
    ( _icosahedron_a, -_icosahedron_b,  0.0),
    ( _icosahedron_a,  _icosahedron_b,  0.0),
    (-_icosahedron_a,  _icosahedron_b,  0.0),
    (-_icosahedron_a, -_icosahedron_b,  0.0),
    ( 0.0,  _icosahedron_a, -_icosahedron_b),
    ( 0.0,  _icosahedron_a,  _icosahedron_b),
    ( 0.0, -_icosahedron_a,  _icosahedron_b),
    ( 0.0, -_icosahedron_a, -_icosahedron_b),
    (-_icosahedron_b,  0.0,  _icosahedron_a),
    ( _icosahedron_b,  0.0,  _icosahedron_a),
    ( _icosahedron_b,  0.0, -_icosahedron_a),
    (-_icosahedron_b,  0.0, -_icosahedron_a),
]

ICOSAHEDRON_FACES = [
    ( 1, 0,10), ( 0, 1, 9), ( 0, 9, 6), ( 9, 8, 6),
    ( 0, 7,10), ( 6, 7, 0), ( 7, 6, 3), ( 6, 8, 3),
    (11,10, 7), ( 7, 3,11), ( 3, 2,11), ( 2, 3, 8),
    (10,11, 4), ( 2, 4,11), ( 5, 4, 2), ( 2, 8, 5),
    ( 4, 1,10), ( 4, 5, 1), ( 5, 9, 1), ( 8, 9, 5),
]

def _cross(a, b):
    return (a[1]*b[2] - a[2]*b[1],
            a[2]*b[0] - a[0]*b[2],
            a[0]*b[1] - a[1]*b[0])

def _dot(a, b):
    return a[0]*b[0] + a[1]*b[1] + a[2]*b[2]

def _norm(v):
    length = math.sqrt(v[0]**2 + v[1]**2 + v[2]**2)
    if length > 0:
        return (v[0]/length, v[1]/length, v[2]/length)
    return v

class GeodesicGridCache:
    """Pre-computed edge midpoints for all levels."""
    def __init__(self, max_level=10):
        self.max_level = max_level
        self.triangles = {}
        for lev in range(max_level):
            n = 20 * (4 ** lev)
            self.triangles[lev] = []
            for idx in range(n):
                c0, c1, c2 = self._corners(lev, idx)
                e0 = _norm((c1[0]+c2[0], c1[1]+c2[1], c1[2]+c2[2]))
                e1 = _norm((c2[0]+c0[0], c2[1]+c0[1], c2[2]+c0[2]))
                e2 = _norm((c0[0]+c1[0], c0[1]+c1[1], c0[2]+c1[2]))
                self.triangles[lev].append((e0, e1, e2))

    def _corners(self, level, index):
        if level <= 0:
            c = ICOSAHEDRON_FACES[index]
            return (ICOSAHEDRON_VERTICES[c[0]],
                    ICOSAHEDRON_VERTICES[c[1]],
                    ICOSAHEDRON_VERTICES[c[2]])
        lev = level - 1
        i = index >> 2
        t = self.triangles[lev][i]
        c0, c1, c2 = self._corners(lev, i)
        sub = index & 3
        if sub == 0:   return (c0, t[2], t[1])
        elif sub == 1: return (t[2], c1, t[0])
        elif sub == 2: return (t[1], t[0], c2)
        else:          return (t[0], t[1], t[2])

    def zone_number(self, x, y, z, level):
        for i in range(20):
            c0, c1, c2 = [ICOSAHEDRON_VERTICES[ICOSAHEDRON_FACES[i][j]] for j in range(3)]
            n0 = _dot(_cross(c0, c1), (x,y,z))
            n1 = _dot(_cross(c1, c2), (x,y,z))
            n2 = _dot(_cross(c2, c0), (x,y,z))
            if n0 >= -1e-10 and n1 >= -1e-10 and n2 >= -1e-10:
                idx = i
                for lev in range(level):
                    t = self.triangles[lev][idx]
                    idx <<= 2
                    if      _dot(_cross(t[1], t[2]), (x,y,z)) <= 1e-10: pass
                    elif    _dot(_cross(t[2], t[0]), (x,y,z)) <= 1e-10: idx += 1
                    elif    _dot(_cross(t[0], t[1]), (x,y,z)) <= 1e-10: idx += 2
                    else:                                                 idx += 3
                return idx
        return 0

    def nr_of_zones(self, level):
        return 20 * (4 ** level) + 1

_grid = GeodesicGridCache(max_level=10)

# ── Helpers ─────────────────────────────────────────────────────

def ra_dec_to_xyz(ra_deg, dec_deg):
    ra  = math.radians(ra_deg)
    dec = math.radians(dec_deg)
    cd  = math.cos(dec)
    return (math.cos(ra) * cd, math.sin(ra) * cd, math.sin(dec))

def safe_float(s):
    if s is None or s == '' or s.lower() == 'null':
        return None
    try:
        return float(s)
    except ValueError:
        return None

def g_to_v(g, c):
    """Gaia G → Johnson V (official Gaia DR3 polynomial, Table 5.9).
    G - V = -0.02704 + 0.01424*C - 0.2156*C^2 + 0.01426*C^3,  C = BP-RP."""
    if c is None:
        return g
    # V = G - (G - V) = G + 0.02704 - 0.01424*C + 0.2156*C^2 - 0.01426*C^3
    return g + 0.02704 - 0.01424 * c + 0.2156 * c * c - 0.01426 * c * c * c

def bp_rp_to_bv(c):
    """BP-RP → B-V for Star3 encoding (clamped to 0-5.375)."""
    # Star3 stores b_v = (bv_raw * 0.025) - 1.0
    # We use BP-RP scaled: bv = c * 0.7 + 0.1  (empirical from catalog comparison)
    bv = c * 0.7 + 0.1
    return max(0.0, min(bv, 5.35))

def write_star3(buf, gaia_id, ra, dec, bv_enc, vmag_enc):
    """Encode Star3 record (16 bytes) into bytearray buffer."""
    # gaia_id (8B, little-endian)
    buf += struct.pack('<q', gaia_id)
    # RA (24-bit, 0.1 arcsec)
    ra_int = int(round(ra * 36000)) & 0xFFFFFF
    buf += struct.pack('<I', ra_int)[:3]
    # Dec (24-bit, 0.1 arcsec, offset +90°)
    dec_int = int(round((dec + 90.0) * 36000)) & 0xFFFFFF
    buf += struct.pack('<I', dec_int)[:3]
    # B-V (8-bit, 0.025 mag, offset -1.0)
    bv_raw = int(round((bv_enc + 1.0) / 0.025))
    buf.append(max(0, min(255, bv_raw)))
    # Vmag (8-bit, 0.02 mag, offset -16.0)
    vmag_raw = int(round((vmag_enc - 16.0) / 0.02))
    buf.append(max(0, min(255, vmag_raw)))
    return buf

# ── CSV iteration ───────────────────────────────────────────────

def list_csv_files(csv_dir):
    """Find all GaiaSource_* files (csv or csv.gz)."""
    files = []
    for fn in os.listdir(csv_dir):
        if fn.startswith("GaiaSource_") and (fn.endswith(".csv.gz") or fn.endswith(".csv")):
            files.append(os.path.join(csv_dir, fn))
    return sorted(files)

def iter_csv_rows(gz_path):
    """Generator: yield parsed rows from a Gaia CSV file (supports .csv and .csv.gz)."""
    if gz_path.endswith(".gz"):
        fh = gzip.open(gz_path, "rt", encoding="utf-8", errors="replace")
    else:
        fh = open(gz_path, "r", encoding="utf-8", errors="replace")
    with fh:
        header = fh.readline().strip()
        while header.startswith("#"):
            header = fh.readline().strip()
        cols = header.split(",")

        col_idx = {}
        for name in ("source_id", "ra", "dec", "phot_g_mean_mag",
                     "phot_bp_mean_mag", "phot_rp_mean_mag"):
            try:
                col_idx[name] = cols.index(name)
            except ValueError:
                col_idx[name] = None

        si, rai, deci = col_idx["source_id"], col_idx["ra"], col_idx["dec"]
        gi, bpi, rpi = col_idx["phot_g_mean_mag"], col_idx["phot_bp_mean_mag"], col_idx["phot_rp_mean_mag"]

        for line in fh:
            line = line.rstrip("\r\n")
            parts = line.split(",")
            if len(parts) != len(cols):
                continue
            try:
                g = safe_float(parts[gi])
                if g is None:
                    continue
                sid = int(parts[si])
                ra  = float(parts[rai])
                dec = float(parts[deci])
                bp  = safe_float(parts[bpi])
                rp  = safe_float(parts[rpi])
                c = bp - rp if (bp is not None and rp is not None) else None
                v = g_to_v(g, c)
                bv = bp_rp_to_bv(c) if c is not None else 0.5
                yield (sid, ra, dec, g, c, v, bv)
            except (ValueError, IndexError):
                continue

# ── Pass 1: Count ───────────────────────────────────────────────

def pass1_count(csv_files, levels, work_dir):
    """Count stars per (level, zone). Returns dict: level_name → [count × nr_zones]."""
    print("\n" + "=" * 60)
    print("PASS 1: Counting stars per zone...")
    print("=" * 60)

    # Initialize counters
    counts = {}
    for cfg in levels:
        nr = _grid.nr_of_zones(cfg["level"])
        counts[cfg["name"]] = [0] * nr

    total = 0
    passed = 0
    for fi, gz_path in enumerate(csv_files):
        fn = os.path.basename(gz_path)
        for sid, ra, dec, g, c, v, bv in iter_csv_rows(gz_path):
            total += 1
            # Assign to level
            level_hit = None
            for cfg in levels:
                if cfg["mag_lo"] <= v < cfg["mag_hi"]:
                    level_hit = cfg
                    break
            if level_hit is None:
                continue
            passed += 1
            # Compute zone
            x, y, z = ra_dec_to_xyz(ra, dec)
            zone = _grid.zone_number(x, y, z, level_hit["level"])
            counts[level_hit["name"]][zone] += 1

        if (fi + 1) % 10 == 0:
            print(f"  [{fi+1}/{len(csv_files)}] total={total:,} passed={passed:,}")

    print(f"\nDone. Total={total:,}  Passed filter={passed:,}")
    for cfg in levels:
        name = cfg["name"]
        total_zone = sum(counts[name])
        nonempty  = sum(1 for c in counts[name] if c > 0)
        print(f"  {name}: {total_zone:,} stars, {nonempty:,} non-empty zones")

    # Save counts for pass 3
    for cfg in levels:
        name = cfg["name"]
        path = os.path.join(work_dir, f"{name}_counts.bin")
        with open(path, "wb") as f:
            # Write zone count header
            f.write(struct.pack('<I', len(counts[name])))
            for cnt in counts[name]:
                f.write(struct.pack('<I', cnt))
        print(f"  Saved counts → {path}")

    return counts

# ── Pass 2: Bucket ──────────────────────────────────────────────

def pass2_bucket(csv_files, cfg, counts, work_dir):
    """Scatter stars from CSVs into bucket files by zone range."""
    name   = cfg["name"]
    level  = cfg["level"]
    n_buckets = cfg["buckets"]
    nr_zones  = _grid.nr_of_zones(level)
    zones_per_bucket = (nr_zones + n_buckets - 1) // n_buckets

    mag_lo = cfg["mag_lo"]
    mag_hi = cfg["mag_hi"]

    print(f"\n{'='*60}")
    print(f"PASS 2: Bucketing {name} (level {level}, {n_buckets} buckets)...")
    print(f"{'='*60}")

    # Open bucket files (binary: sequence of (zone_id, vmag_raw, bv_raw, ra_int, dec_int, gaia_id))
    bucket_dir = os.path.join(work_dir, f"{name}_buckets")
    os.makedirs(bucket_dir, exist_ok=True)
    bucket_files = []
    bucket_writers = []
    for b in range(n_buckets):
        path = os.path.join(bucket_dir, f"bucket_{b:04d}.dat")
        bucket_files.append(path)
        bucket_writers.append(open(path, "wb", buffering=1024*1024))  # 1MB buffer

    # Bucket record format: 24 bytes = zone_id(4) + vmag_i16(2) + bv_i16(2) + ra_i32(4) + dec_i32(4) + gaia_id(8)
    # (vmag in milli-mag, bv in milli-mag, ra/dec in mas)
    fmt_bucket = struct.Struct('<I h h i i q')

    total = 0
    for fi, gz_path in enumerate(csv_files):
        for sid, ra, dec, g, c, v, bv in iter_csv_rows(gz_path):
            if not (mag_lo <= v < mag_hi):
                continue
            total += 1

            x, y, z = ra_dec_to_xyz(ra, dec)
            zone = _grid.zone_number(x, y, z, level)

            # Encode
            vmag_i16 = int(round(v * 1000))
            ra_i32   = int(round(ra * 3600000))
            dec_i32  = int(round(dec * 3600000))
            bv_i16   = int(round(bv * 1000))

            b = zone // zones_per_bucket
            if b >= n_buckets:
                b = n_buckets - 1

            bucket_writers[b].write(fmt_bucket.pack(zone, vmag_i16, bv_i16, ra_i32, dec_i32, sid))

        if (fi + 1) % 10 == 0:
            print(f"  [{fi+1}/{len(csv_files)}] {name}: {total:,} stars")

    for fh in bucket_writers:
        fh.close()

    print(f"{name}: {total:,} stars written to {n_buckets} buckets")
    return bucket_files, bucket_dir

# ── Pass 3: Sort per bucket → .cat ──────────────────────────────

def pass3_sort_write(bucket_files, cfg, counts, work_dir, out_dir):
    """Read each bucket, sort by (zone, vmag), write to final .cat."""
    name   = cfg["name"]
    level  = cfg["level"]
    n_buckets = cfg["buckets"]
    nr_zones  = _grid.nr_of_zones(level)

    print(f"\n{'='*60}")
    print(f"PASS 3: Sorting + writing {name} ({n_buckets} buckets → .cat)...")
    print(f"{'='*60}")

    cat_path = os.path.join(out_dir, f"{name}_{level}2v0_1.cat")
    fmt_bucket = struct.Struct('<I h h i i q')

    # Allocate file + write header + zone table
    zone_sizes = counts  # [nr_zones]
    total_stars = sum(zone_sizes)
    zone_table_start = 28
    star_data_start = zone_table_start + nr_zones * 4
    file_size = star_data_start + total_stars * STAR3_REC_SIZE

    with open(cat_path, "wb") as fcat:
        # Write header (28 bytes)
        fcat.write(struct.pack('<I', FILE_MAGIC))          # magic
        fcat.write(struct.pack('<I', 2))                    # type = Star3
        fcat.write(struct.pack('<I', 0))                    # major
        fcat.write(struct.pack('<I', 1))                    # minor
        fcat.write(struct.pack('<I', level))                # level
        fcat.write(struct.pack('<i', int(cfg["mag_lo"] * 1000)))  # mag_min (millimag)
        fcat.write(struct.pack('<f', CATALOG_EPOCH))        # epoch

        # Zone table: star count per zone
        for sz in zone_sizes:
            fcat.write(struct.pack('<I', sz))

        # Pre-allocate rest to file size (seek to end, write 1 byte)
        fcat.seek(file_size - 1)
        fcat.write(b'\x00')

    # Process each bucket
    zone_offset = star_data_start
    for zone_id in range(nr_zones):
        zone_offset += (zone_sizes[zone_id-1] * STAR3_REC_SIZE) if zone_id > 0 else 0

    # Actually, it's easier to compute offsets once, then seek-write per zone
    offsets = [star_data_start]
    for z in range(1, nr_zones):
        offsets.append(offsets[-1] + zone_sizes[z-1] * STAR3_REC_SIZE)

    for bi, bf_path in enumerate(bucket_files):
        # Read entire bucket into memory
        records = []
        with open(bf_path, "rb") as fb:
            data = fb.read()
            n_records = len(data) // fmt_bucket.size
            for i in range(n_records):
                off = i * fmt_bucket.size
                zone, vmag_i16, bv_i16, ra_i32, dec_i32, gaia_id = fmt_bucket.unpack_from(data, off)
                records.append((zone, vmag_i16, bv_i16, ra_i32, dec_i32, gaia_id))

        # Sort by (zone, vmag ascending = bright first)
        records.sort(key=lambda r: (r[0], r[1]))

        # Group by zone and write to .cat
        with open(cat_path, "r+b") as fcat:
            buf = bytearray()
            current_zone = -1

            for zone, vmag_i16, bv_i16, ra_i32, dec_i32, gaia_id in records:
                if zone != current_zone:
                    # Flush previous zone buffer
                    if current_zone >= 0 and len(buf) > 0:
                        fcat.seek(offsets[current_zone])
                        fcat.write(buf)
                    current_zone = zone
                    buf.clear()

                # Encode Star3 record
                gaia_v = float(vmag_i16) / 1000.0
                bv_v   = float(bv_i16) / 1000.0
                ra_d   = float(ra_i32) / 3600000.0
                dec_d  = float(dec_i32) / 3600000.0

                # Star3 encoding
                buf += struct.pack('<q', gaia_id)
                ra_enc  = int(round(ra_d * 36000)) & 0xFFFFFF
                buf += struct.pack('<I', ra_enc)[:3]
                dec_enc = int(round((dec_d + 90.0) * 36000)) & 0xFFFFFF
                buf += struct.pack('<I', dec_enc)[:3]
                bv_enc  = max(0, min(255, int(round((bv_v + 1.0) / 0.025))))
                buf.append(bv_enc)
                vmag_enc = max(0, min(255, int(round((gaia_v - 16.0) / 0.02))))
                buf.append(vmag_enc)

            # Flush last zone
            if current_zone >= 0 and len(buf) > 0:
                fcat.seek(offsets[current_zone])
                fcat.write(buf)

        print(f"  Bucket {bi+1}/{n_buckets} ({bf_path}): {len(records):,} records")

        # Delete bucket file
        os.remove(bf_path)

    # Cleanup bucket directory
    bucket_dir = os.path.dirname(bucket_files[0])
    try:
        os.rmdir(bucket_dir)
    except OSError:
        pass

    print(f"  → {cat_path} ({os.path.getsize(cat_path):,} bytes)")

# ── Main ────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(description="Gaia DR3 CSV → Stellarium .cat (low-memory)")
    parser.add_argument("--csv-dir", default=r"F:\gaiadr3", help="Directory with GaiaSource_*.csv.gz files")
    parser.add_argument("--out-dir", default=r"F:\gaiadr3\output", help="Output directory for .cat files")
    parser.add_argument("--work-dir", default=None, help="Working directory for temp files (default: out-dir)")
    parser.add_argument("--levels", nargs="+", default=["stars_9","stars_10"],
                        help="Which levels to process (stars_9, stars_10)")
    args = parser.parse_args()

    csv_dir = args.csv_dir
    out_dir = args.out_dir
    work_dir = args.work_dir or out_dir

    os.makedirs(out_dir, exist_ok=True)
    os.makedirs(work_dir, exist_ok=True)

    # Filter levels
    active_levels = [cfg for cfg in LEVELS if cfg["name"] in args.levels]

    # Find CSV files
    csv_files = list_csv_files(csv_dir)
    print(f"Found {len(csv_files)} CSV files in {csv_dir}")

    if not csv_files:
        print("No CSV files found!", file=sys.stderr)
        sys.exit(1)

    # Pass 1: Count
    counts = pass1_count(csv_files, active_levels, work_dir)

    # Pass 2 & 3 per level
    for cfg in active_levels:
        name = cfg["name"]
        csv_count = counts[name]
        bucket_files, bucket_dir = pass2_bucket(csv_files, cfg, csv_count, work_dir)
        pass3_sort_write(bucket_files, cfg, csv_count, work_dir, out_dir)

    print("\n" + "=" * 60)
    print("DONE. Output files in", out_dir)
    print("=" * 60)


if __name__ == "__main__":
    main()
