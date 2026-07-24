#!/usr/bin/env python3
"""Analyze cmpcat output: cross-zone stars, boundary-magnitude, missing-BP/RP stats."""

import sys
import re

def parse_line(line):
    """Parse new cmpcat format: id z=N RA=x DEC=y V=x BV=x [tag]"""
    # [only in A/B]  lines: have RA= DEC=
    m = re.match(r'(\d+)\s+z=(\d+)\s+RA=([\d.+-]+)\s+DEC=([\d.+-]+)\s+V=([\d.+-]+)\s+BV=([\d.+-]+)\s+\[only in ([AB])\]', line)
    if m:
        return (int(m.group(1)), int(m.group(2)),
                float(m.group(3)), float(m.group(4)),
                float(m.group(5)), float(m.group(6)),
                f"only in {m.group(7)}")
    return None

def read_cmpcat_output(path):
    only_a = {}
    only_b = []
    with open(path, 'r') as f:
        for line in f:
            r = parse_line(line)
            if r is None:
                continue
            gaia, zone, ra, dec, vmag, bv, tag = r
            if tag == 'only in A':
                only_a[gaia] = (zone, ra, dec, vmag, bv)
            elif tag == 'only in B':
                only_b.append((gaia, zone, ra, dec, vmag, bv))
    return only_a, only_b


def match_cross_zone(only_a, only_b):
    cross = []
    unmatched_b = []
    for gaia, zb, ra, dec, vb, bvb in only_b:
        a = only_a.pop(gaia, None)
        if a is not None:
            za, _, _, va, bva = a
            cross.append((gaia, za, zb, va, vb, bva, bvb))
        else:
            unmatched_b.append((gaia, zb, ra, dec, vb, bvb))
    unmatched_a = [(gaia, zone, ra, dec, vmag, bv)
                   for gaia, (zone, ra, dec, vmag, bv) in only_a.items()]
    return cross, unmatched_a, unmatched_b


def split_boundary(items, mag_lo, mag_hi, margin):
    boundary, other = [], []
    for g, z, ra, dec, v, bv in items:
        if abs(v - mag_lo) <= margin or abs(v - mag_hi) <= margin:
            boundary.append((g, z, ra, dec, v, bv))
        else:
            other.append((g, z, ra, dec, v, bv))
    return boundary, other


def report_boundary_counts(unmatched_a, unmatched_b, mag_lo, mag_hi, margin):
    a_boundary, a_other = split_boundary(unmatched_a, mag_lo, mag_hi, margin)
    b_boundary, b_other = split_boundary(unmatched_b, mag_lo, mag_hi, margin)
    print(f"\nWithin {margin} mag of boundary {mag_lo} or {mag_hi}:")
    print(f"  only in A: {len(a_boundary)} / {len(unmatched_a)}")
    print(f"  only in B: {len(b_boundary)} / {len(unmatched_b)}")
    print(f"Out of boundary:")
    print(f"  only in A: {len(a_other)}")
    print(f"  only in B: {len(b_other)}")
    return a_boundary, a_other, b_boundary, b_other


def print_sample_list(title, items):
    if not items:
        return
    print(f"\n{title}:")
    for g, z, ra, dec, v, bv in items[:10]:
        print(f"  {g}  z={z}  RA={ra:.6f} DEC={dec:+.6f} V={v:.3f} BV={bv:.3f}")


def print_cross_zone_samples(cross):
    if not cross:
        return
    print(f"\nSample cross-zone (first 10):")
    for g, za, zb, va, vb, _, _ in cross[:10]:
        print(f"  {g}  A:z={za} V={va:.3f}  B:z={zb} V={vb:.3f}  dV={va-vb:.4f}")


def count_no_bv(items):
    # Star2: missing BP-RP is encoded as 32767 (B-V = 32.767 in printed output).
    sentinel = 32767 / 1000.0
    return sum(1 for _, _, _, _, _, bv in items if abs(bv - sentinel) < 1e-6)


def print_no_bv_summary(unmatched_a, unmatched_b):
    no_bv_a = count_no_bv(unmatched_a)
    no_bv_b = count_no_bv(unmatched_b)
    print(f"  of which no BV:")
    print(f"    only in A: {no_bv_a}")
    print(f"    only in B: {no_bv_b}")


def main(path, mag_lo=None, mag_hi=None, margin=0.05):
    only_a, only_b = read_cmpcat_output(path)
    cross, unmatched_a, unmatched_b = match_cross_zone(only_a, only_b)

    print(f"Cross-zone matches:   {len(cross)}")
    print(f"Unmatched only in A:  {len(unmatched_a)}")
    print(f"Unmatched only in B:  {len(unmatched_b)}")

    print_no_bv_summary(unmatched_a, unmatched_b)

    if mag_lo is not None and mag_hi is not None:
        a_boundary, a_other, b_boundary, b_other = report_boundary_counts(
            unmatched_a, unmatched_b, mag_lo, mag_hi, margin)
        print_sample_list("Sample only-in-B outside boundary", b_other)
        print_sample_list("Sample only-in-B at boundary", b_boundary)

    print_cross_zone_samples(cross)

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: python analyze_only.py <cmpcat_output.txt> [mag_lo] [mag_hi] [margin]")
        print("Example: python analyze_only.py out.txt 15.5 16.75 0.005")
        sys.exit(1)
    main(sys.argv[1],
         float(sys.argv[2]) if len(sys.argv) > 2 else None,
         float(sys.argv[3]) if len(sys.argv) > 3 else None,
         float(sys.argv[4]) if len(sys.argv) > 4 else 0.05)
