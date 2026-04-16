#!/usr/bin/env python3
"""
fetch_asteroid_ephemeris.py
===========================
Fetches multi-epoch Keplerian orbital elements for numbered asteroids from
JPL Horizons and writes them to asteroid_ephemeris.json for use with
Stellarium's extended ephemeris system.

BACKGROUND
----------
Stellarium normally stores a single set of orbital elements per asteroid,
valid only near the epoch of those elements (typically within one year).
This script generates a table of snapshots spread over a long time window.
Stellarium's MinorPlanet class then picks the nearest snapshot and propagates
with Kepler's equation from there, giving accurate positions across decades or
centuries without any manual element updates.

WHY JPL HORIZONS INSTEAD OF MPC/SBDB
-------------------------------------
The MPC and JPL Small-Body Database (SBDB) only supply elements at a single
fixed epoch per asteroid (the current MPC epoch or the JPL solution epoch).
JPL Horizons numerically integrates the full N-body perturbed solution and can
return osculating elements at any requested date, which is exactly what the
multi-epoch table requires.

QUICK START
-----------
    pip install requests
    python fetch_asteroid_ephemeris.py

This fetches the first 500 numbered asteroids with 6-month epoch intervals
spanning 200 years (1926-2126).

Place the output file in the user data folder (same location as
ssystem_minor.ini) and restart Stellarium.  If the file is absent, Stellarium
falls back to normal single-epoch orbits transparently.

PARAMETERS
----------
  --count N
      How many asteroids to fetch, counting sequentially from 1 (or from
      --start-at).  Asteroids are identified by their MPC number, so --count 500
      fetches (1) Ceres through (500) Selinur.  Default: 500.

  --numbers N[,N,...]
      Fetch a specific, explicit list of asteroid numbers instead of a
      sequential range.  Useful for topping up a file with particular objects
      or for testing.  Overrides --count and --start-at.
      Example: --numbers 1,2,3,4,433,1036,3552

  --epochs N
      Number of epoch snapshots per asteroid for the default time window.
      Together with --span this controls the time resolution:
          interval = span / (epochs - 1)
      More epochs = smoother apparent motion when time-scrubbing in Stellarium,
      at the cost of a larger file and longer download time.
      Rule of thumb: aim for an interval shorter than the orbital period of the
      fastest target in your set.  Eros (period 1.76 yr) needs <1-yr intervals;
      for main-belt asteroids (periods 3-6 yr) 1-yr intervals are fine.
      Default: 400  (= ~6-month intervals over the default 200-year span).

  --span YEARS
      Total time window width in years, centred on --center.
      Default: 200  (i.e. center-100 to center+100).

  --center YEAR
      The midpoint year of the time window.  Fractional years are allowed.
      Shifting the centre toward the future gives better coverage for
      forward-looking use; shifting toward the past covers historical work.
      Default: 2026.0

  --out FILE
      Output filename.  If the file already exists (e.g. from a previous
      interrupted run) the script resumes automatically, skipping asteroids
      already present.  Default: asteroid_ephemeris.json

  --delay SECS
      Pause between consecutive Horizons API requests, in seconds.  Also
      applied between the chunk requests that make up a single large asteroid
      fetch.  Increase this if you see repeated HTTP 429 (rate-limit) errors.
      Default: 0.6

  --start-at N
      Begin the sequential fetch at asteroid number N instead of 1.
      Ignored when --numbers is used.  Combine with --out and an existing
      partial file to resume a previously interrupted run.
      Default: 1

  --debug
      Fetch asteroid 1 with just two epochs, print the raw Horizons JSON
      response to stdout, and exit.  Useful for diagnosing API or parsing
      problems without waiting for a full run.

HORIZONS API LIMITS
-------------------
Horizons caps TLIST (list of individual dates) requests at 80 time steps per
call.  The script splits larger epoch lists into chunks of 80 automatically,
so you can request any number of epochs without manual intervention.

Each asteroid costs:
    ceil(epochs / 80) data requests  +  1 info request

For 400 epochs: 5 + 1 = 6 requests per asteroid.
For 600 epochs: 8 + 1 = 9 requests per asteroid.
At 0.6 s delay, 500 asteroids at 400 epochs takes roughly 30 minutes.

SCALING EXAMPLES
----------------
  First 500 asteroids, default settings (~52 MB, ~20 min):
      python fetch_asteroid_ephemeris.py

  First 1000 asteroids, same density (~104 MB, ~40 min):
      python fetch_asteroid_ephemeris.py --count 1000

  Tighter 3-month intervals over 200 years (~104 MB, ~40 min):
      python fetch_asteroid_ephemeris.py --epochs 800

  800-year window for everything, 1-year intervals (~52 MB, ~40 min):
      python fetch_asteroid_ephemeris.py --span 800 --epochs 800 --center 2000

  Specific asteroids only:
      python fetch_asteroid_ephemeris.py --numbers 1,2,3,4,433 --no-special

  Resume an interrupted run:
      python fetch_asteroid_ephemeris.py --start-at 247

REQUIREMENTS
------------
    pip install requests

Horizons API reference: https://ssd-api.jpl.nasa.gov/doc/horizons.html
"""

import argparse
import json
import math
import sys
import time
import requests
from datetime import datetime, timezone

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------
J2000_JDE    = 2451545.0
DAYS_PER_YEAR = 365.25
HORIZONS_URL  = "https://ssd.jpl.nasa.gov/api/horizons.api"

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def jde_to_decimal_year(jde: float) -> float:
    return 2000.0 + (jde - J2000_JDE) / DAYS_PER_YEAR


def decimal_year_to_jde(year: float) -> float:
    return J2000_JDE + (year - 2000.0) * DAYS_PER_YEAR


def build_epoch_list(center_year: float, span_years: float,
                     n_epochs: int) -> list[float]:
    """Return n_epochs JDE values evenly spaced across the time window."""
    start = decimal_year_to_jde(center_year - span_years / 2.0)
    end   = decimal_year_to_jde(center_year + span_years / 2.0)
    if n_epochs == 1:
        return [decimal_year_to_jde(center_year)]
    step = (end - start) / (n_epochs - 1)
    return [start + i * step for i in range(n_epochs)]

# ---------------------------------------------------------------------------
# Horizons query — epochs fetched in chunks and merged
# ---------------------------------------------------------------------------

# JPL Horizons silently caps TLIST requests at 80 time steps.
# We split larger epoch lists into chunks of this size.
TLIST_CHUNK_SIZE = 80


def _parse_horizons_csv(result_text: str, asteroid_number: int) -> list[dict] | None:
    """Parse the $$SOE...$$EOE CSV block from a Horizons JSON response."""

    lines = result_text.splitlines()

    soe_idx = next((i for i, l in enumerate(lines) if "$$SOE" in l), None)
    eoe_idx = next((i for i, l in enumerate(lines) if "$$EOE" in l), None)

    if soe_idx is None or eoe_idx is None:
        print(f"\n  WARNING: Missing $$SOE/$$EOE markers for asteroid "
              f"{asteroid_number}", file=sys.stderr)
        return None

    header_line = None
    for i in range(soe_idx - 1, max(soe_idx - 10, -1), -1):
        if "JDTDB" in lines[i]:
            header_line = lines[i]
            break

    if header_line is None:
        print(f"\n  WARNING: Could not find JDTDB header for asteroid "
              f"{asteroid_number}. Lines near $$SOE:", file=sys.stderr)
        for i in range(max(0, soe_idx - 5), min(len(lines), soe_idx + 5)):
            print(f"    [{i}]: {repr(lines[i])}", file=sys.stderr)
        return None

    col_names = [c.strip() for c in header_line.split(",")]
    data_rows = [l for l in lines[soe_idx + 1 : eoe_idx] if l.strip()]

    if not data_rows:
        print(f"\n  WARNING: No data rows for asteroid {asteroid_number}",
              file=sys.stderr)
        return None

    results = []
    for row in data_rows:
        vals = [v.strip() for v in row.split(",")]
        if len(vals) < len(col_names):
            continue
        d = dict(zip(col_names, vals))
        try:
            results.append({
                "epoch_jde":           float(d["JDTDB"]),
                "semi_major_axis":     float(d["A"]),
                "eccentricity":        float(d["EC"]),
                "inclination":         float(d["IN"]),
                "ascending_node":      float(d["OM"]),
                "arg_of_pericenter":   float(d["W"]),
                "mean_anomaly":        float(d["MA"]),
                "mean_motion":         float(d["N"]),
                "time_at_pericenter":  float(d["Tp"]),
                "pericenter_distance": float(d["QR"]),
            })
        except (KeyError, ValueError) as exc:
            print(f"\n  WARNING: Bad row for asteroid {asteroid_number}: "
                  f"{exc}", file=sys.stderr)
            continue

    return results if results else None


def _fetch_chunk(asteroid_number: int,
                 chunk_jdes: list[float],
                 session: requests.Session) -> list[dict] | None:
    """Single Horizons TLIST request for one chunk (<= TLIST_CHUNK_SIZE epochs)."""

    params = {
        "format":     "json",
        "COMMAND":    f"'{asteroid_number};'",
        "EPHEM_TYPE": "ELEMENTS",
        "CENTER":     "'@sun'",
        "REF_PLANE":  "ECLIPTIC",
        "REF_SYSTEM": "ICRF",
        "OUT_UNITS":  "AU-D",
        "TP_TYPE":    "ABSOLUTE",
        "CSV_FORMAT": "YES",
        "OBJ_DATA":   "YES",
        "TLIST":      " ".join(f"{jde:.4f}" for jde in chunk_jdes),
    }

    try:
        resp = session.get(HORIZONS_URL, params=params, timeout=60)
        resp.raise_for_status()
        data = resp.json()
    except requests.exceptions.HTTPError as exc:
        print(f"\n  WARNING: HTTP {exc.response.status_code} for asteroid "
              f"{asteroid_number}: {exc}", file=sys.stderr)
        return None
    except Exception as exc:
        print(f"\n  WARNING: Request error for asteroid {asteroid_number}: "
              f"{exc}", file=sys.stderr)
        return None

    result_text = data.get("result", "")

    for bad in ("No ephemeris", "No body", "cannot be matched", "ERROR", "No object"):
        if bad in result_text:
            snippet = result_text[:200].replace("\n", " ").strip()
            print(f"\n  WARNING: Horizons error for asteroid "
                  f"{asteroid_number}: {snippet}", file=sys.stderr)
            return None

    return _parse_horizons_csv(result_text, asteroid_number)


def fetch_horizons_elements(asteroid_number: int,
                             epoch_jdes: list[float],
                             session: requests.Session,
                             delay: float = 0.0) -> list[dict] | None:
    """
    Fetch osculating orbital elements from JPL Horizons for all requested JDEs.

    Automatically splits the epoch list into chunks of TLIST_CHUNK_SIZE (80)
    to respect Horizons' per-request limit, merging results transparently.
    Pass delay (seconds) to sleep between chunk requests.
    """
    all_results: list[dict] = []

    chunks = [epoch_jdes[i : i + TLIST_CHUNK_SIZE]
              for i in range(0, len(epoch_jdes), TLIST_CHUNK_SIZE)]

    for idx, chunk in enumerate(chunks):
        chunk_results = _fetch_chunk(asteroid_number, chunk, session)
        if chunk_results is None:
            return None
        all_results.extend(chunk_results)
        if delay > 0.0 and idx < len(chunks) - 1:
            time.sleep(delay)

    return all_results if all_results else None


def parse_object_info(result_text: str, number: int) -> dict:
    """Extract name, H and G from a Horizons result header block."""
    name  = str(number)
    h_mag = 99.0
    slope = 0.15

    for line in result_text.splitlines():
        if "Target body name:" in line:
            part = line.split("Target body name:")[1].strip()
            name = part.split("{")[0].strip()
        if "H=" in line and "G=" in line:
            try:
                h_mag = float(line.split("H=")[1].strip().split()[0])
            except (IndexError, ValueError):
                pass
            try:
                slope = float(line.split("G=")[1].strip().split()[0])
            except (IndexError, ValueError):
                pass

    return {"name": name, "absolute_magnitude": h_mag, "slope_parameter": slope}

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="Fetch multi-epoch asteroid orbital elements from JPL Horizons"
    )
    parser.add_argument("--count",    type=int,   default=500)
    parser.add_argument("--numbers",  type=str,   default=None,
                        help="Comma-separated asteroid numbers, e.g. 1,2,4,433,1036. "
                             "Overrides --count and --start-at.")
    parser.add_argument("--epochs",   type=int,   default=400,
                        help="Epochs per asteroid for normal asteroids (default 400 = "
                             "6-month intervals over 200 years).")
    parser.add_argument("--center",   type=float, default=2026.0)
    parser.add_argument("--span",     type=float, default=200.0)
    parser.add_argument("--out",      type=str,   default="asteroid_ephemeris.json")
    parser.add_argument("--delay",    type=float, default=0.6)
    parser.add_argument("--start-at", type=int,   default=1, dest="start_at")
    parser.add_argument("--debug",    action="store_true",
                        help="Dump raw Horizons response for asteroid 1 and exit.")
    args = parser.parse_args()

    default_epochs = build_epoch_list(args.center, args.span, args.epochs)

    # Build the target list — explicit numbers take priority over range
    if args.numbers:
        target_numbers = sorted(set(int(x.strip()) for x in args.numbers.split(",")))
    else:
        target_numbers = list(range(args.start_at, args.count + 1))

    print("Stellarium Extended Asteroid Ephemeris Fetcher  (Horizons backend)")
    if args.numbers:
        print(f"  Asteroids : {target_numbers}")
    else:
        print(f"  Asteroids : {args.start_at} – {args.count}")
    print(f"  Epochs    : {args.epochs}  "
          f"({jde_to_decimal_year(default_epochs[0]):.1f}–"
          f"{jde_to_decimal_year(default_epochs[-1]):.1f})")
    print(f"  Output    : {args.out}")
    print()

    output = {
        "format":      "stellarium_asteroid_ephemeris",
        "version":     1,
        "generator":   "fetch_asteroid_ephemeris.py (Horizons)",
        "generated":   datetime.now(timezone.utc).isoformat(),
        "center_year": args.center,
        "span_years":  args.span,
        "epoch_jdes":  default_epochs,
        "asteroids":   []
    }

    # Resume support: load existing output file if present
    existing_numbers: set[int] = set()
    try:
        with open(args.out, "r") as f:
            existing = json.load(f)
            output["asteroids"] = existing.get("asteroids", [])
            existing_numbers = {a["number"] for a in output["asteroids"]}
            print(f"Resuming: {len(existing_numbers)} asteroids already in file.")
    except FileNotFoundError:
        pass

    session = requests.Session()
    session.headers.update({"User-Agent": "Stellarium-EphemerisFetcher/2.0"})

    # Debug mode: dump raw Horizons response and exit
    if args.debug:
        print("DEBUG MODE: fetching asteroid 1 with 2 epochs and printing raw response...")
        test_epochs = [default_epochs[0], default_epochs[-1]]
        tlist_str = " ".join(f"{jde:.4f}" for jde in test_epochs)
        params = {
            "format": "json", "COMMAND": "'1;'", "EPHEM_TYPE": "ELEMENTS",
            "CENTER": "'@sun'", "REF_PLANE": "ECLIPTIC", "REF_SYSTEM": "ICRF",
            "OUT_UNITS": "AU-D", "TP_TYPE": "ABSOLUTE", "CSV_FORMAT": "YES",
            "OBJ_DATA": "YES", "TLIST": tlist_str,
        }
        r = session.get(HORIZONS_URL, params=params, timeout=60)
        print(f"HTTP status: {r.status_code}")
        result = r.json().get("result", "")
        for i, line in enumerate(result.splitlines()):
            print(f"{i:4d}: {repr(line)}")
        return

    success_count = len(existing_numbers)
    total = len(target_numbers)

    for idx, num in enumerate(target_numbers, 1):
        if num in existing_numbers:
            continue

        print(f"[{idx:4d}/{total}] asteroid {num:5d}  ", end="", flush=True)

        ast_epochs = default_epochs
        expected   = args.epochs

        # Fetch all epochs, chunked automatically if > 80
        epoch_data = fetch_horizons_elements(num, ast_epochs, session, delay=args.delay)
        time.sleep(args.delay)

        if not epoch_data:
            print("SKIPPED (no data)")
            continue
        if len(epoch_data) != expected:
            print(f"SKIPPED (got {len(epoch_data)}/{expected} epochs)")
            continue

        # Parse name and H/G out of the result we already have
        # (fetch_horizons_elements returned it; repeat tiny call for info)
        # To avoid an extra round-trip, reuse OBJ_DATA from the same call.
        # The name/H/G come from the header which we parse here via a second
        # minimal call only if we couldn't get them from the first.
        # For simplicity: do a tiny dummy call to get just the header.
        info_params = {
            "format":     "json",
            "COMMAND":    f"'{num};'",
            "EPHEM_TYPE": "ELEMENTS",
            "CENTER":     "'@sun'",
            "REF_PLANE":  "ECLIPTIC",
            "OUT_UNITS":  "AU-D",
            "OBJ_DATA":   "YES",
            "TLIST":      f"{ast_epochs[len(ast_epochs)//2]:.4f}",  # single midpoint
        }
        info = {"name": str(num), "absolute_magnitude": 99.0, "slope_parameter": 0.15}
        try:
            r = session.get(HORIZONS_URL, params=info_params, timeout=30)
            r.raise_for_status()
            info = parse_object_info(r.json().get("result", ""), num)
        except Exception:
            pass
        time.sleep(args.delay)

        entry = {
            "number":             num,
            "name":               info["name"],
            "absolute_magnitude": info["absolute_magnitude"],
            "slope_parameter":    info["slope_parameter"],
            "epochs":             epoch_data,
        }
        output["asteroids"].append(entry)
        success_count += 1
        print(f"OK  ({info['name']},  H={info['absolute_magnitude']:.1f})")

        # Save progress every 10 asteroids so we can resume if interrupted
        if idx % 10 == 0:
            with open(args.out, "w") as f:
                json.dump(output, f, indent=2)
            print(f"  --> saved ({success_count} asteroids so far)")

    # Final save
    with open(args.out, "w") as f:
        json.dump(output, f, indent=2)

    import os
    size = os.path.getsize(args.out)
    print()
    print(f"Done.  {success_count} asteroids written to {args.out}")
    print(f"File size: {size/1024:.0f} KB uncompressed")


if __name__ == "__main__":
    main()
