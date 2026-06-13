#!/usr/bin/env python3
"""
Generate Gaia DR3 gaia_source CSV download URLs for Free Download Manager.
Uses subprocess + curl to avoid urllib issues with the mirror.
"""

import subprocess
import re
import argparse
import sys

MIRROR_BASE = "http://159.226.170.25/mirror/Gaia/dr3/gaia_source/"


def fetch_urls():
    """Scrape all GaiaSource_*.csv.gz URLs from the mirror via curl."""
    print(f"Fetching file list from {MIRROR_BASE} ...")
    result = subprocess.run(
        ["curl.exe", "-s", MIRROR_BASE],
        capture_output=True, text=True, timeout=60
    )
    if result.returncode != 0:
        print(f"curl failed: {result.stderr}", file=sys.stderr)
        return []

    html = result.stdout
    pattern = re.compile(r'href="(GaiaSource_\d+-\d+\.csv\.gz)"')
    files = pattern.findall(html)
    files.sort()
    return files


def main():
    parser = argparse.ArgumentParser(
        description="Generate Gaia DR3 CSV download URL list"
    )
    parser.add_argument(
        "-o", "--output",
        default="gaia_dr3_download_urls.txt",
        help="Output file (default: gaia_dr3_download_urls.txt)"
    )
    args = parser.parse_args()

    files = fetch_urls()
    if not files:
        print("No files found. Mirror may be down.", file=sys.stderr)
        sys.exit(1)

    with open(args.output, "w") as f:
        for fn in files:
            f.write(MIRROR_BASE + fn + "\n")

    total_gb = len(files) * 0.22
    total_tb = total_gb / 1024
    print(f"\nGenerated {len(files)} URLs -> {args.output}")
    print(f"Estimated total size: ~{total_gb:.0f} GB ({total_tb:.1f} TB)")
    print(f"\nImport into Free Download Manager: File -> Import -> URL List")
    print(f"Set download folder to F:\\gaiadr3")


if __name__ == "__main__":
    main()
