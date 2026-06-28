#!/usr/bin/env python3
"""Extract 9 Gaia columns to compact binary (60B/star). Multiprocessing."""

import sys, os, struct, gzip, math, glob
from multiprocessing import Pool

COLS = ['source_id','ra','dec','parallax','parallax_error','pmra','pmdec','phot_g_mean_mag','bp_rp']
REC_FMT = '<q d d d f d d f f'
REC_SZ = 60

def process_file(args):
    csv_path, out_dir = args
    name = os.path.basename(csv_path).replace('.csv.gz', '.bin')
    out_path = os.path.join(out_dir, name)
    if os.path.exists(out_path):
        return (csv_path, 0, True)

    idx = {}
    with gzip.open(csv_path, 'rt') as f:
        for line in f:
            if line.startswith('#'): continue
            idx = {c: i for i, c in enumerate(line.strip().split(','))}
            break
        for c in COLS:
            if c not in idx:
                return (csv_path, 0, False, f"missing column {c}")

        buf = bytearray()
        count = 0
        for line in f:
            if line.startswith('#'): continue
            row = line.strip().split(',')
            def fv(n):
                v = row[idx[n]].strip()
                return float(v) if v and v != 'null' else float('nan')
            try:
                sid = int(row[idx['source_id']])
            except (ValueError, IndexError):
                continue
            rec = struct.pack(REC_FMT, sid,
                fv('ra'), fv('dec'), fv('parallax'), fv('parallax_error'),
                fv('pmra'), fv('pmdec'), fv('phot_g_mean_mag'), fv('bp_rp'))
            buf.extend(rec)
            count += 1

    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    with open(out_path, 'wb') as f:
        f.write(buf)
    return (csv_path, count, True)

def main():
    if len(sys.argv) < 3:
        print(f"Usage: python {sys.argv[0]} <csv_dir> <out_dir> [workers]")
        sys.exit(1)
	csv_dir = sys.argv[1]
	out_dir = sys.argv[2]
    workers = int(sys.argv[3]) if len(sys.argv) > 3 else os.cpu_count()

    os.makedirs(out_dir, exist_ok=True)
    files = sorted(glob.glob(os.path.join(csv_dir, 'GaiaSource_*.csv.gz')))
    print(f"Found {len(files)} files, {workers} workers, {REC_SZ}B/star\n")

	total = 0
	skipped = 0
	errors = 0
    try:
        from tqdm import tqdm
        pbar = tqdm(total=len(files), unit='file', ncols=80)
    except ImportError:
        pbar = None

    with Pool(workers) as pool:
        for r in pool.imap_unordered(process_file, [(f, out_dir) for f in files]):
            if pbar: pbar.update(1)
            if len(r) == 3:
                total += r[1]
                if r[2]: skipped += 1
            else:
                print(f"\n  ERROR: {r[0]}: {r[3]}")
                errors += 1

    if pbar: pbar.close()
    print(f"Done: {total} stars, {skipped} skipped, {errors} errors")

if __name__ == '__main__':
    main()
