#!/usr/bin/env python3
"""
SkyChart Gaia .dat -> Stellarium .cat converter (optimized 2-pass).
Pass 1: scan + count + write per-worker intermediate files (24B records, no recomputation)
Pass 2: route intermediate files into bucket files -> sort -> write .cat

Usage: python gaia_skychart_v2.py --skychart DIR --out-dir DIR
"""

import struct, os, sys, math, argparse, signal
from multiprocessing import Pool, cpu_count
from concurrent.futures import ProcessPoolExecutor

try:
    from numba import njit
    HAS_NUMBA = True
except ImportError:
    HAS_NUMBA = False
    def njit(*args, **kwargs):
        return lambda f: f

FILE_MAGIC      = 0x835F040A
CATALOG_EPOCH   = 2457389.0
STAR3_REC_SIZE  = 16
SKYCHART_REC    = 38
BUCKET_FMT      = struct.Struct('<I h h i i q')  # zone(4)+vmag_i16(2)+bv_i16(2)+ra_i32(4)+dec_i32(4)+gaia_id(8)=24B

LEVELS = [
    {"name": "stars_9",  "level": 9,  "mag_lo": 18.0, "mag_hi": 20.0, "buckets": 12},
    {"name": "stars_10", "level": 10, "mag_lo": 20.0, "mag_hi": 22.0, "buckets": 48},
]

# ── Geodesic Grid (lazy + per-file cache) ───────────────────────

PHI  = (1.0 + math.sqrt(5.0))/2.0
NORM = math.sqrt(1.0 + PHI*PHI)
_a, _b = PHI/NORM, 1.0/NORM

ICOSAHEDRON_VERTICES = [
    ( _a,-_b,0.0),( _a, _b,0.0),(-_a, _b,0.0),(-_a,-_b,0.0),
    (0.0, _a,-_b),(0.0, _a, _b),(0.0,-_a, _b),(0.0,-_a,-_b),
    (-_b,0.0, _a),( _b,0.0, _a),( _b,0.0,-_a),(-_b,0.0,-_a),
]
ICOSAHEDRON_FACES = [
    (1,0,10),(0,1,9),(0,9,6),(9,8,6),(0,7,10),(6,7,0),(7,6,3),(6,8,3),
    (11,10,7),(7,3,11),(3,2,11),(2,3,8),(10,11,4),(2,4,11),(5,4,2),(2,8,5),
    (4,1,10),(4,5,1),(5,9,1),(8,9,5),
]

@njit(cache=True)
def _cross(a,b): return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])
@njit(cache=True)
def _dot(a,b):   return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]
@njit(cache=True)
def _norm(v):
    ln = math.sqrt(v[0]**2+v[1]**2+v[2]**2)
    return (v[0]/ln, v[1]/ln, v[2]/ln) if ln>0 else v

class GeodesicGrid:
    def __init__(self):
        self._cache = {}
    def _corners(self, level, index):
        key = (level, index)
        if key in self._cache:
            return self._cache[key]
        if level <= 0:
            c = ICOSAHEDRON_FACES[index]
            r = (ICOSAHEDRON_VERTICES[c[0]], ICOSAHEDRON_VERTICES[c[1]], ICOSAHEDRON_VERTICES[c[2]])
        else:
            lev, i = level-1, index>>2
            c0,c1,c2 = self._corners(lev, i)
            e0 = _norm((c1[0]+c2[0],c1[1]+c2[1],c1[2]+c2[2]))
            e1 = _norm((c2[0]+c0[0],c2[1]+c0[1],c2[2]+c0[2]))
            e2 = _norm((c0[0]+c1[0],c0[1]+c1[1],c0[2]+c1[2]))
            s = index & 3
            if   s==0: r=(c0,e2,e1)
            elif s==1: r=(e2,c1,e0)
            elif s==2: r=(e1,e0,c2)
            else:      r=(e0,e1,e2)
        self._cache[key] = r
        return r
    def clear_cache(self):
        self._cache.clear()
    def zone_number(self, x, y, z, level):
        for i in range(20):
            c0,c1,c2 = [ICOSAHEDRON_VERTICES[ICOSAHEDRON_FACES[i][j]] for j in range(3)]
            if _dot(_cross(c0,c1),(x,y,z))>=-1e-10 and _dot(_cross(c1,c2),(x,y,z))>=-1e-10 and _dot(_cross(c2,c0),(x,y,z))>=-1e-10:
                idx = i
                for lev in range(level):
                    c0,c1,c2 = self._corners(lev, idx)
                    e0 = _norm((c1[0]+c2[0],c1[1]+c2[1],c1[2]+c2[2]))
                    e1 = _norm((c2[0]+c0[0],c2[1]+c0[1],c2[2]+c0[2]))
                    e2 = _norm((c0[0]+c1[0],c0[1]+c1[1],c0[2]+c1[2]))
                    idx <<= 2
                    if      _dot(_cross(e1,e2),(x,y,z))<=1e-10: pass
                    elif    _dot(_cross(e2,e0),(x,y,z))<=1e-10: idx+=1
                    elif    _dot(_cross(e0,e1),(x,y,z))<=1e-10: idx+=2
                    else:                                        idx+=3
                return idx
        return 0
    @staticmethod
    def nr_of_zones(level):
        return 20*(4**level)+1

# ── G->V conversion ────────────────────────────────────────────

@njit(cache=True)
def g_to_v(g, c):
    if c is None:
        return g
    return g + 0.02704 - 0.01424*c + 0.2156*c*c - 0.01426*c*c*c

@njit(cache=True)
def bp_rp_to_bv(c):
    """BP-RP -> Johnson B-V via SkyChart's two-step polynomial (cu_catalog.pas:6500-6530).
    Step 1: BP-RP -> Tycho2 BT-VT (Gaia EDR3, valid -0.3 < BP-RP < 3.0)
    Step 2: BT-VT -> B-V (Mamajek 2006 piecewise)"""
    if c is None:
        return 0.5
    bt_vt = -0.006482 + 0.7865*c - 0.3631*c*c + 0.93192*c*c*c - 0.4843*c*c*c*c + 0.06814*c*c*c*c*c
    if -0.25 < bt_vt < 0.5:
        bv = bt_vt - 0.006 - 0.1069*bt_vt + 0.1459*bt_vt*bt_vt
    elif 0.5 <= bt_vt < 2.0:
        bv = bt_vt - 0.007813*bt_vt - 0.1489*bt_vt*bt_vt + 0.03384*bt_vt*bt_vt*bt_vt
    else:
        bv = 0.850 * bt_vt
    return max(0.0, min(bv, 5.35))

# ── SkyChart I/O ────────────────────────────────────────────────

def list_skychart_files(base_dir):
    files = []
    for sub in ['gaia1','gaia2','gaia3','gaia4']:
        d = os.path.join(base_dir, sub)
        if not os.path.isdir(d):
            continue
        for zdir in os.listdir(d):
            zp = os.path.join(d, zdir)
            if not os.path.isdir(zp):
                continue
            for fn in os.listdir(zp):
                if fn.endswith('.dat'):
                    files.append(os.path.join(zp, fn))
    return sorted(files)

def iter_skychart(dat_path):
    try:
        with open(dat_path, 'rb') as f:
            data = f.read()
    except OSError as e:
        print(f"  WARNING: skipping bad file {dat_path}: {e}", file=sys.stderr)
        return  # returns None -> caller must check
    n = len(data) // SKYCHART_REC
    if n == 0:
        yield from ()
        return
    for i in range(n):
        off = i*SKYCHART_REC
        gi = struct.unpack_from('<h', data, off+16)[0]; g = gi/1000.0
        bi = struct.unpack_from('<h', data, off+18)[0]; bp= bi/1000.0
        ri = struct.unpack_from('<h', data, off+20)[0]; rp= ri/1000.0
        if abs(bi)>=30000 or abs(ri)>=30000:
            bp, rp = None, None
        sid = struct.unpack_from('<Q',data,off+8)[0]
        ra  = struct.unpack_from('<I',data,off)[0]/3600000.0
        dec = struct.unpack_from('<I',data,off+4)[0]/3600000.0 - 90.0
        yield (sid, ra, dec, g, bp-rp if bp is not None else None, bp, rp)

# ── Pass 1: scan + count + write intermediate files ─────────────

def _pass1_worker(args):
    """Worker: count per zone and write per-chunk intermediate file."""
    fpaths, levels, chunk_idx, work_dir = args
    grid = GeodesicGrid()

    # Intermediate file: all stars that pass filter, in raw bucket format (24B)
    # Intermediate files: one per (chunk, level)
    inter_paths = {cfg["name"]: os.path.join(work_dir, f"inter_{chunk_idx:05d}_{cfg['name']}.dat") for cfg in levels}
    inter_fhs = {}
    for cfg in levels:
        inter_fhs[cfg["name"]] = open(inter_paths[cfg["name"]], 'wb', buffering=8*1024*1024)

    counts = {cfg["name"]: {} for cfg in levels}
    for fpath in fpaths:
        it = iter_skychart(fpath)
        if it is None:
            continue
        for sid, ra, dec, g, c, bp, rp in it:
            if c is None:
                continue
            v = g_to_v(g, c)
            for cfg in levels:
                if cfg["mag_lo"] <= v < cfg["mag_hi"]:
                    x, y, z = (math.cos(ra_m:=math.radians(ra))*math.cos(d_m:=math.radians(dec)),
                               math.sin(ra_m)*math.cos(d_m), math.sin(d_m))
                    zone = grid.zone_number(x, y, z, cfg["level"])
                    d = counts[cfg["name"]]
                    d[zone] = d.get(zone, 0) + 1
                    bv = bp_rp_to_bv(c)
                    inter_fhs[cfg["name"]].write(BUCKET_FMT.pack(
                        zone, int(round(v*1000)), int(round(bv*1000)),
                        int(round(ra*3600000)), int(round(dec*3600000)), sid))
                    break
        grid.clear_cache()

    for fh in inter_fhs.values():
        fh.close()
    return (counts, inter_paths)

def pass1(files, levels, work_dir, n_workers=6):
    """Multiprocess: count + write intermediate files in one pass."""
    chunk_size = max(200, (len(files)+n_workers*10-1)//(n_workers*10))
    chunks = []
    for i in range(0, len(files), chunk_size):
        chunks.append((files[i:i+chunk_size], levels, len(chunks), work_dir))

    print(f"\n{'='*60}")
    print(f"PASS 1: Counting + writing intermediate files ({n_workers} workers, {len(chunks)} chunks)...")

    full_counts = {cfg["name"]: [0]*GeodesicGrid.nr_of_zones(cfg["level"]) for cfg in levels}
    inter_files = {cfg["name"]: [] for cfg in levels}
    signal.signal(signal.SIGINT, signal.SIG_DFL)

    with Pool(n_workers) as pool:
        for i, (worker_counts, inter_paths) in enumerate(pool.imap_unordered(_pass1_worker, chunks)):
            for cfg in levels:
                arr = full_counts[cfg["name"]]
                for zone, cnt in worker_counts[cfg["name"]].items():
                    arr[zone] += cnt
                inter_files[cfg["name"]].append(inter_paths[cfg["name"]])
            done = (i+1)*chunk_size
            if done > len(files): done = len(files)
            total_passed = sum(sum(c for c in full_counts[cfg["name"]]) for cfg in levels)
            print(f"  [{done}/{len(files)}] {100.0*done/len(files):.1f}%  passed={total_passed:,}")

    for cfg in levels:
        name = cfg["name"]
        s = sum(full_counts[name])
        ne = sum(1 for c in full_counts[name] if c>0)
        print(f"  {name}: {s:,} stars, {ne:,} non-empty zones")
        # Save counts for future skip-pass1
        cp = os.path.join(work_dir, f"{name}_counts.bin")
        with open(cp, "wb") as fh:
            fh.write(struct.pack('<I', len(full_counts[name])))
            for cnt in full_counts[name]:
                fh.write(struct.pack('<I', cnt))
    return full_counts, inter_files

# ── Pass 2: route intermediate -> buckets -> sort -> .cat ───────

def _sort_bucket(args):
    """Process-level worker: read bucket, sort, write .cat (called via ProcessPoolExecutor)."""
    bi, bpath, cat_path, offsets, zone_count = args
    n = len(zone_count)
    with open(bpath, 'rb') as fb:
        data = fb.read()
    n_rec = len(data) // BUCKET_FMT.size
    records = [(0, 0, b'')] * n_rec
    for i in range(n_rec):
        off = i * BUCKET_FMT.size
        zone = struct.unpack_from('<I', data, off)[0]
        vmag = struct.unpack_from('<h', data, off+4)[0]
        records[i] = (zone, vmag, data[off:off+BUCKET_FMT.size])
    records.sort(key=lambda r: (r[0], r[1]))
    del data  # free 1.2 GB raw bytes before sort temp memory kicks in

    with open(cat_path, 'r+b') as fcat:
        buf = bytearray()
        cur_zone = -1
        for zone, _vmag, raw in records:
            if zone != cur_zone:
                if cur_zone >= 0 and len(buf) > 0:
                    fcat.seek(offsets[cur_zone])
                    fcat.write(buf)
                cur_zone = zone
                buf.clear()
                if cur_zone >= n:
                    break
            if cur_zone >= n:
                continue
            gid   = struct.unpack_from('<q', raw, 16)[0]
            ra_i  = struct.unpack_from('<i', raw, 8)[0]
            dec_i = struct.unpack_from('<i', raw, 12)[0]
            bv_i  = struct.unpack_from('<h', raw, 6)[0]
            v_d   = struct.unpack_from('<h', raw, 4)[0] / 1000.0
            bv_d  = bv_i / 1000.0
            ra_d  = ra_i / 3600000.0
            dec_d = dec_i / 3600000.0
            buf += struct.pack('<q', gid)
            buf += struct.pack('<I', int(round(ra_d*36000)) & 0xFFFFFF)[:3]
            buf += struct.pack('<I', int(round((dec_d+90.0)*36000)) & 0xFFFFFF)[:3]
            buf.append(max(0, min(255, int(round((bv_d+1.0)/0.025)))))
            buf.append(max(0, min(255, int(round((v_d-16.0)/0.02)))))
        if cur_zone >= 0 and len(buf) > 0 and cur_zone < n:
            fcat.seek(offsets[cur_zone])
            fcat.write(buf)
    os.remove(bpath)
    return (bi, n_rec)

def pass2_to_cat(inter_files, cfg, counts, out_dir, bucket_dir=None):
    """Route intermediate -> buckets -> parallel sort -> .cat."""
    name, level, n_buckets = cfg["name"], cfg["level"], cfg["buckets"]
    nr_zones = GeodesicGrid.nr_of_zones(level)
    zones_per_bucket = (nr_zones+n_buckets-1)//n_buckets
    if bucket_dir is None:
        bucket_dir = out_dir

    print(f"\nPASS 2: Routing {name} ({len(inter_files)} files -> {n_buckets} buckets, dir={bucket_dir})...")

    cat_path = os.path.join(out_dir, f"{name}_{level}2v0_1.cat")
    total_stars = sum(counts)
    offsets = [28 + nr_zones*4]
    for z in range(1, nr_zones):
        offsets.append(offsets[-1] + counts[z-1]*STAR3_REC_SIZE)

    # Step 1: Write .cat header + zone table
    with open(cat_path, 'wb') as fcat:
        fcat.write(struct.pack('<I', FILE_MAGIC))
        fcat.write(struct.pack('<I', 2))
        fcat.write(struct.pack('<I', 0)); fcat.write(struct.pack('<I', 1))
        fcat.write(struct.pack('<I', level))
        fcat.write(struct.pack('<i', int(cfg["mag_lo"]*1000)))
        fcat.write(struct.pack('<f', CATALOG_EPOCH))
        for sz in counts:
            fcat.write(struct.pack('<I', sz))
        fcat.seek(offsets[-1] + counts[-1]*STAR3_REC_SIZE - 1)
        fcat.write(b'\x00')

    # Step 2: Route intermediate files -> bucket files
    import shutil
    tmp_dir = os.path.join(bucket_dir, f"{name}_buckets")
    os.makedirs(tmp_dir, exist_ok=True)
    bucket_fhs = [open(os.path.join(tmp_dir, f"bucket_{b:04d}.dat"), 'wb', buffering=8*1024*1024) for b in range(n_buckets)]
    routed = 0
    for fi, ipath in enumerate(inter_files):
        if not os.path.exists(ipath):
            continue
        with open(ipath, 'rb') as sf:
            data = sf.read()
        for i in range(len(data)//BUCKET_FMT.size):
            off = i*BUCKET_FMT.size
            zone = struct.unpack_from('<I', data, off)[0]
            b = zone // zones_per_bucket
            if b >= n_buckets: b = n_buckets-1
            bucket_fhs[b].write(data[off:off+BUCKET_FMT.size])
            routed += 1
        if (fi+1)%50 == 0:
            print(f"  routed {routed:,} records from {fi+1}/{len(inter_files)} files")
    for fh in bucket_fhs:
        fh.close()
    print(f"  total routed: {routed:,} records")

    # Step 3: Sort buckets in parallel processes and write .cat
    n_sort_workers = min(n_buckets, 4)  # 4 processes ≈ 58 GB peak, safe for 128 GB
    print(f"  sorting {n_buckets} buckets ({n_sort_workers} processes) -> {cat_path}")

    tasks = [(bi, os.path.join(tmp_dir, f"bucket_{bi:04d}.dat"), cat_path, offsets, counts)
             for bi in range(n_buckets)]
    with ProcessPoolExecutor(max_workers=n_sort_workers) as executor:
        for bi, n_rec in executor.map(_sort_bucket, tasks):
            print(f"  bucket {bi+1}/{n_buckets}: {n_rec:,} records")

    shutil.rmtree(tmp_dir, ignore_errors=True)

# ── Main ────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(description="SkyChart Gaia .dat -> Stellarium .cat (optimized 2-pass)")
    parser.add_argument('--skychart', required=True)
    parser.add_argument('--out-dir', required=True)
    parser.add_argument('--work-dir', default=None)
    parser.add_argument('--workers', type=int, default=6)
    parser.add_argument('--bucket-dir', default=None, help='Temp directory for bucket files (default: out-dir)')
    parser.add_argument('--skip-pass1', action='store_true')
    args = parser.parse_args()

    out_dir  = args.out_dir
    work_dir = args.work_dir or out_dir
    os.makedirs(out_dir, exist_ok=True)
    os.makedirs(work_dir, exist_ok=True)

    # Find files in gaia3+gaia4 (G<21)
    files = []
    for sub in ['gaia3','gaia4']:
        d = os.path.join(args.skychart, sub)
        if os.path.isdir(d):
            for zdir in sorted(os.listdir(d)):
                zp = os.path.join(d, zdir)
                if os.path.isdir(zp):
                    for fn in sorted(os.listdir(zp)):
                        if fn.endswith('.dat'):
                            files.append(os.path.join(zp, fn))
    print(f"Found {len(files)} .dat files in gaia3+gaia4")

    # Pass 1: count + intermediate files (skip if --skip-pass1)
    if not args.skip_pass1:
        counts, inter_files = pass1(files, LEVELS, work_dir, args.workers)
    else:
        print("Skipping Pass 1, loading counts...")
        counts = {}
        inter_files = {}
        for cfg in LEVELS:
            name = cfg["name"]
            nr_zones = GeodesicGrid.nr_of_zones(cfg["level"])
            cp = os.path.join(work_dir, f"{name}_counts.bin")
            if os.path.exists(cp):
                # Fast path: read pre-saved counts
                with open(cp, "rb") as fh:
                    nz = struct.unpack('<I', fh.read(4))[0]
                    counts[name] = [struct.unpack('<I', fh.read(4))[0] for _ in range(nz)]
            else:
                # Slow path: scan intermediate files to rebuild counts
                counts[name] = [0] * nr_zones
                print(f"  {name}: no counts.bin, scanning intermediate files...")
            iplist = []
            for fn in sorted(os.listdir(work_dir)):
                if fn.startswith("inter_") and fn.endswith(f"_{name}.dat"):
                    fpath = os.path.join(work_dir, fn)
                    iplist.append(fpath)
                    if not os.path.exists(cp):
                        with open(fpath, "rb") as fh:
                            data = fh.read()
                        for i in range(len(data) // BUCKET_FMT.size):
                            zone = struct.unpack_from('<I', data, i * BUCKET_FMT.size)[0]
                            counts[name][zone] += 1
            inter_files[name] = sorted(iplist)
            s = sum(counts[name])
            print(f"  {name}: {s:,} stars, {len(iplist)} intermediate files")

    # Pass 2: route -> sort -> .cat
    for cfg in LEVELS:
        pass2_to_cat(inter_files[cfg["name"]], cfg, counts[cfg["name"]], out_dir, args.bucket_dir)

    print("\nDONE. Output in", out_dir)

if __name__ == '__main__':
    main()
