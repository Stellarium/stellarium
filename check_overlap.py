#!/usr/bin/env python3
"""Check for overlapping source_ids between .cat files (supports Star2 and Star3)."""
import struct, sys, os, time

files = sys.argv[1:] if len(sys.argv)>1 else ['stars_7_1v0_4.cat','stars_8_82v0_1.cat','stars_9_92v0_1.cat','stars_10_102v0_1.cat']

def load_ids_set(path, max_ids=None):
    """Load ALL source_ids into a Python set. Returns (set, rec_size, total)."""
    print(f'Loading {os.path.basename(path)} into set...', flush=True)
    ids = set()
    with open(path, 'rb') as f:
        h = f.read(28)
        magic = struct.unpack_from('<I', h, 0)[0]
        typ   = struct.unpack_from('<I', h, 4)[0]
        level = struct.unpack_from('<I', h, 16)[0]
        nz    = 20*(4**level)+1
        rec_size = 48 if typ==0 else (32 if typ==1 else 16)
        f.seek(28)
        zs = f.read(nz*4)
        total = 0
        for z in range(nz):
            cnt = struct.unpack_from('<I', zs, z*4)[0]
            if cnt == 0:
                continue
            for _ in range(cnt):
                sid = struct.unpack_from('<q', f.read(rec_size), 0)[0]
                if sid != 0:
                    ids.add(sid)
                total += 1
            if max_ids and total >= max_ids:
                break
    print(f'  {len(ids):,} unique / {total:,} total (rec_size={rec_size}B)')
    return ids

def check_against(path, *ref_sets, names=None):
    """Stream through path and count overlaps with each ref_set. Returns None (no set built)."""
    print(f'Checking {os.path.basename(path)} against {len(ref_sets)} reference sets...', flush=True)
    overlaps = [0] * len(ref_sets)
    total = 0
    with open(path, 'rb') as f:
        h = f.read(28)
        typ   = struct.unpack_from('<I', h, 4)[0]
        level = struct.unpack_from('<I', h, 16)[0]
        nz    = 20*(4**level)+1
        rec_size = 48 if typ==0 else (32 if typ==1 else 16)
        f.seek(28)
        zs = f.read(nz*4)
        for z in range(nz):
            cnt = struct.unpack_from('<I', zs, z*4)[0]
            if cnt == 0:
                continue
            for _ in range(cnt):
                sid = struct.unpack_from('<q', f.read(rec_size), 0)[0]
                if sid == 0:
                    continue
                total += 1
                for i, ref in enumerate(ref_sets):
                    if sid in ref:
                        overlaps[i] += 1
            if total % 50000000 == 0:
                print(f'  {total:,} checked...', flush=True)
    print(f'  {total:,} total')
    if names:
        for i, n in enumerate(names):
            print(f'  Overlap with {n}: {overlaps[i]:,}')
    return overlaps

t0 = time.time()

# Strategy: load smallest file first as reference set
# Check headers to determine sizes
sizes = []
for f in files:
    with open(f, 'rb') as fh:
        h = fh.read(28); nz = 20*(4**struct.unpack_from('<I',h,16)[0])+1
        fh.seek(28); zs = fh.read(nz*4)
        sizes.append((f, sum(struct.unpack_from('<I',zs,i*4)[0] for i in range(nz))))
        print(f'  {os.path.basename(f)}: {sizes[-1][1]:,} stars')
print()

# Load stars_7 as reference (smallest)
stars7 = load_ids_set(files[0])

# Check stars_8, stars_9, stars_10 against stars_7
print()
for f in files[1:]:
    overlaps = check_against(f, stars7, names=[os.path.basename(files[0])])
    if overlaps[0] > 0:
        print(f'  *** OVERLAP DETECTED: {overlaps[0]} stars in both {os.path.basename(f)} and {os.path.basename(files[0])} ***')

# Now load stars_8 and stars_9 as reference, check stars_9 and stars_10 against them
print(f'\n--- Cross-checking {len(files)-1} levels ---')
loaded = [(files[0], stars7)]
for i in range(1, len(files)):
    if i == 1:  # stars_8: load set
        s = load_ids_set(files[i])
        loaded.append((files[i], s))
    else:  # stars_9, stars_10: stream-check against previous sets
        ref_sets = [r[1] for r in loaded]
        ref_names = [os.path.basename(r[0]) for r in loaded]
        overlaps = check_against(files[i], *ref_sets, names=ref_names)
        for j, ov in enumerate(overlaps):
            if ov > 0:
                print(f'  *** OVERLAP: {ov} stars in both {os.path.basename(files[i])} and {ref_names[j]} ***')

print(f'\nDone in {time.time()-t0:.1f}s')
