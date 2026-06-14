#!/usr/bin/env python3
"""Generate MD5 checksums for all SkyChart .dat files (multithreaded)."""
import os, hashlib, sys
from concurrent.futures import ThreadPoolExecutor

def md5_file(path):
    try:
        with open(path, 'rb') as f:
            h = hashlib.md5(f.read()).hexdigest()
        return h, path, None
    except Exception as e:
        return None, path, str(e)

def main():
    base = sys.argv[1] if len(sys.argv) > 1 else '.'
    out  = sys.argv[2] if len(sys.argv) > 2 else 'gaia_md5.txt'

    # Collect all .dat files
    files = []
    for sub in ['gaia1','gaia2','gaia3','gaia4']:
        d = os.path.join(base, sub)
        if not os.path.isdir(d):
            continue
        for zdir in os.listdir(d):
            zp = os.path.join(d, zdir)
            if not os.path.isdir(zp):
                continue
            for fn in os.listdir(zp):
                if fn.endswith('.dat'):
                    files.append(os.path.join(zp, fn))
    files.sort()
    print(f'{len(files)} files found')

    with ThreadPoolExecutor(max_workers=12) as pool, open(out, 'w') as fh:
        done = 0
        for md5, path, err in pool.map(md5_file, files):
            done += 1
            if err:
                fh.write(f'ERROR  {path}  {err}\n')
                print(f'  ERROR {path}')
            else:
                fh.write(f'{md5}  {path}\n')
            if done % 5000 == 0:
                print(f'  {done}/{len(files)} ({100.0*done/len(files):.1f}%)')

    print(f'Done -> {out}')

if __name__ == '__main__':
    main()
