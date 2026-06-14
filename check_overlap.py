#!/usr/bin/env python3
"""Check if any source_id appears in multiple .cat files."""
import struct, sys

files = sys.argv[1:] if len(sys.argv)>1 else ['output/stars_8_2v0_3.cat', 'output/stars_9_92v0_1.cat']

def load_ids(path):
    print(f'Reading {path}...')
    ids = set()
    with open(path, 'rb') as f:
        h = f.read(28)
        level = struct.unpack_from('<I', h, 16)[0]
        nz = 20*(4**level)+1
        f.seek(28)
        zs = f.read(nz*4)
        total = 0
        for z in range(nz):
            cnt = struct.unpack_from('<I', zs, z*4)[0]
            if cnt == 0:
                continue
            for _ in range(cnt):
                sid = struct.unpack_from('<q', f.read(16), 0)[0]
                if sid != 0:
                    ids.add(sid)
                total += 1
    print(f'  {len(ids):,} unique IDs ({total:,} total records)')
    return ids

all_ids = [load_ids(f) for f in files]

for i in range(len(all_ids)):
    for j in range(i+1, len(all_ids)):
        overlap = all_ids[i] & all_ids[j]
        oi = all_ids[i] - all_ids[j]
        oj = all_ids[j] - all_ids[i]
        print(f'\n{files[i]} vs {files[j]}:')
        print(f'  OVERLAP: {len(overlap):,}')
        if overlap:
            print(f'  Examples: {list(overlap)[:3]}')
        print(f'  Only in first:  {len(oi):,}')
        print(f'  Only in second: {len(oj):,}')
        print(f'  Disjoint: {len(overlap)==0}')
