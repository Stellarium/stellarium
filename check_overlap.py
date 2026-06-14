#!/usr/bin/env python3
"""Check if any source_id appears in both stars_8 and stars_9 .cat files."""
import struct, os, sys

stars_8 = sys.argv[1] if len(sys.argv)>1 else 'output/stars_8_2v0_3.cat'
stars_9 = sys.argv[2] if len(sys.argv)>2 else 'output/stars_9_92v0_1.cat'

def load_ids(path):
    print(f'Reading {path}...')
    ids = set()
    with open(path, 'rb') as f:
        h = f.read(28)
        level = struct.unpack_from('<I', h, 16)[0]
        nz = 20*(4**level)+1
        f.seek(28)
        zs = f.read(nz*4)
        base = 28 + nz*4
        off = base
        total = 0
        for z in range(nz):
            cnt = struct.unpack_from('<I', zs, z*4)[0]
            if cnt == 0:
                continue
            for _ in range(cnt):
                f.seek(off)
                sid = struct.unpack_from('<q', f.read(16), 0)[0]
                ids.add(sid)
                total += 1
            off += cnt*16
    print(f'  {len(ids):,} unique IDs ({total:,} total records)')
    return ids

ids8 = load_ids(stars_8)
ids9 = load_ids(stars_9)

overlap = ids8 & ids9
only8 = ids8 - ids9
only9 = ids9 - ids8

print(f'\nstars_8 total: {len(ids8):,}')
print(f'stars_9 total: {len(ids9):,}')
print(f'OVERLAP (in both): {len(overlap):,}')
print(f'Only in stars_8: {len(only8):,}')
print(f'Only in stars_9: {len(only9):,}')

if overlap:
    print(f'\n⚠️  {len(overlap):,} stars appear in BOTH files! Examples:')
    for sid in list(overlap)[:5]:
        print(f'  {sid}')
else:
    print(f'\n✅ No overlap — levels are perfectly disjoint.')
