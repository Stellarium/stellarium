#!/usr/bin/env python3
"""
compare_ssystem.py -- Compare Stellarium object positions with JPL Horizons
OBSERVER data.

For each moon and planet in ssystem_major.ini, RA/Dec positions are fetched
from JPL Horizons and from the locally running Stellarium instance over a
configurable time range (default: 2 years, weekly). The angular error in
arcminutes is plotted per object and saved as a multi-page PDF.

Observer location: read from Stellarium at startup and passed to JPL as
geodetic surface coordinates, so both sides use exactly the same observer.

Usage:
    python3 compare_ssystem.py ../data/ssystem_major.ini
    python3 compare_ssystem.py ../data/ssystem_major.ini himalia
    python3 compare_ssystem.py ../data/ssystem_major.ini --years 1 \
        --out my_compare.pdf
"""

import argparse
import csv
import json
import math
import re
import sys
import time
from datetime import datetime, timezone
from urllib.error import URLError  # noqa: F401  (used by callers via re-raise)
from urllib.parse import urlencode
from urllib.request import Request, urlopen

import requests

try:
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.dates as mdates
    import matplotlib.pyplot as plt
    import matplotlib.ticker
    from matplotlib.backends.backend_pdf import PdfPages
except ImportError:
    print(
        "ERROR: matplotlib not found. Install with: pip install matplotlib",
        file=sys.stderr,
    )
    sys.exit(1)

# -- Configuration ------------------------------------------------------------

STELLARIUM_URL = "http://localhost:8090"

# Seconds to wait after setting Stellarium time before querying position.
# Increase on slow machines if positions appear stale.
STELLARIUM_DELAY = 0.05

# -- JPL Horizons object IDs --------------------------------------------------

OBJECT_IDS = {
    "sun": "10",
    "mercury": "199",
    "earth": "399",
    "moon": "301",
    "mars": "499",
    "jupiter": "599",
    "saturn": "699",
    "neptune": "899",
    "pluto": "999",
    "phobos": "401",
    "deimos": "402",
    "io": "501",
    "europa": "502",
    "ganymede": "503",
    "callisto": "504",
    "amalthea": "505",
    "himalia": "506",
    "elara": "507",
    "pasiphae": "508",
    "sinope": "509",
    "lysithea": "510",
    "carme": "511",
    "ananke": "512",
    "leda": "513",
    "thebe": "514",
    "adrastea": "515",
    "metis": "516",
    "mimas": "601",
    "enceladus": "602",
    "tethys": "603",
    "dione": "604",
    "rhea": "605",
    "titan": "606",
    "hyperion": "607",
    "iapetus": "608",
    "phoebe": "609",
    "janus": "610",
    "epimetheus": "611",
    "helene": "612",
    "telesto": "613",
    "calypso": "614",
    "atlas": "615",
    "prometheus": "616",
    "pandora": "617",
    "pan": "618",
    "ariel": "701",
    "umbriel": "702",
    "titania": "703",
    "oberon": "704",
    "miranda": "705",
    "cordelia": "706",
    "ophelia": "707",
    "cressida": "709",
    "desdemona": "710",
    "juliet": "711",
    "puck": "715",
    "triton": "801",
    "nereid": "802",
    "naiad": "803",
    "thalassa": "804",
    "despina": "805",
    "galatea": "806",
    "larissa": "807",
    "proteus": "808",
    "halimede": "809",
    "psamathe": "810",
    "sao": "811",
    "laomedeia": "812",
    "neso": "813",
    "charon": "901",
    "nix": "902",
    "hydra": "903",
    "kerberos": "904",
    "styx": "905",
}

PARENT_IDS = {
    "Mercury": "199",
    "Venus": "299",
    "Earth": "399",
    "Mars": "499",
    "Jupiter": "599",
    "Saturn": "699",
    "Uranus": "799",
    "Neptune": "899",
    "Pluto": "999",
}

# -- Time utilities -----------------------------------------------------------


def julian_date_now():
    """Return today's date as Julian Date at 0:00 UTC."""
    dt = datetime.now(timezone.utc)
    y, m, d = dt.year, dt.month, dt.day
    if m <= 2:
        y -= 1
        m += 12
    a = y // 100
    b = 2 - a + a // 4
    return (
        int(365.25 * (y + 4716))
        + int(30.6001 * (m + 1))
        + d + b - 1524.5
    )


def jd_to_datetime(jd):
    """Convert Julian Date to UTC datetime."""
    jd2 = jd + 0.5
    z = int(jd2)
    f = jd2 - z
    if z < 2299161:
        a = z
    else:
        alpha = int((z - 1867216.25) / 36524.25)
        a = z + 1 + alpha - alpha // 4
    b = a + 1524
    c = int((b - 122.1) / 365.25)
    d_int = int(365.25 * c)
    e = int((b - d_int) / 30.6001)
    day = b - d_int - int(30.6001 * e)
    month = e - 1 if e < 14 else e - 13
    year = c - 4716 if month > 2 else c - 4715
    fh = f * 24
    h = int(fh)
    fm = (fh - h) * 60
    m_ = int(fm)
    s = int((fm - m_) * 60)
    return datetime(year, month, day, h, m_, s, tzinfo=timezone.utc)


def jd_to_date_str(jd):
    """Format Julian Date as 'YYYY-MM-DD' for JPL API calls."""
    return jd_to_datetime(jd).strftime("%Y-%m-%d")


# -- INI parser (shared logic with fetch_ssystem_major.py) -------------------


def split_value_line(line):
    m = re.match(r"^(\s*([^=\s#]+)\s*=\s*)(.*?)(\s*(?:#.*)?\n?)$", line)
    if not m:
        return None
    return m.group(1), m.group(2), m.group(3), m.group(4)


def read_sections(lines):
    sections = []
    current = None
    for i, line in enumerate(lines):
        m = re.match(r"^\s*\[([^\]]+)\]", line)
        if m:
            current = {
                "name": m.group(1),
                "start": i,
                "end": len(lines),
                "keys": {},
            }
            sections.append(current)
            if len(sections) > 1:
                sections[-2]["end"] = i
        if current:
            p = split_value_line(line)
            if p:
                current["keys"][p[1]] = i
    return sections


def get_objects(lines, requested_object=None):
    """
    Return all sections with type=moon or type=planet that have a known
    OBJECT_ID. Moons additionally require a known parent. Planets have no
    parent. If requested_object is given, return only that one section.
    """
    sections = read_sections(lines)
    result = []

    for sec in sections:
        name = sec["name"]
        keys = sec["keys"]

        if requested_object and name.lower() != requested_object.lower():
            continue

        if "type" not in keys:
            continue

        type_val = split_value_line(lines[keys["type"]])
        if not type_val:
            continue
        obj_type = type_val[2].strip()

        if obj_type == "moon":
            if "parent" not in keys:
                continue
            parent = split_value_line(lines[keys["parent"]])[2].strip()
            if parent not in PARENT_IDS:
                continue
        elif obj_type == "planet":
            parent = None
        else:
            continue

        if name.lower() not in OBJECT_IDS:
            continue

        result.append({"name": name, "type": obj_type, "parent": parent})

    return result


# -- Stellarium API -----------------------------------------------------------


def stellarium_status():
    """Return the full status JSON from Stellarium."""
    url = f"{STELLARIUM_URL}/api/main/status"
    with urlopen(url, timeout=5) as r:
        return json.loads(r.read().decode("utf-8"))


def stellarium_check():
    """Raise URLError if Stellarium is not reachable."""
    stellarium_status()


def stellarium_get_time():
    """
    Return Stellarium's current time as Julian Date.
    Reads from GET /api/main/status -> time.jday
    (GET /api/main/time is not supported; only POST for setting.)
    """
    data = stellarium_status()
    time_block = data.get("time", {})
    if isinstance(time_block, dict):
        return float(time_block["jday"])
    return float(data["jday"])  # fallback for older Stellarium versions


def stellarium_get_property(prop_id):
    """
    Return the current value of a Stellarium property.
    Uses GET /api/stelproperty/list (no single-property GET endpoint
    available).
    """
    response = requests.get(
        f"{STELLARIUM_URL}/api/stelproperty/list", timeout=10
    )
    return response.json().get(prop_id, {}).get("value")


def stellarium_set_property(prop_id, value):
    """
    Set a Stellarium property via POST /api/stelproperty/set.
    value is passed as a string ('true' / 'false' for booleans).
    """
    params = urlencode({"id": prop_id, "value": str(value).lower()})
    url = f"{STELLARIUM_URL}/api/stelproperty/set?{params}"
    req = Request(url, data=b"", method="POST")
    with urlopen(req, timeout=10) as r:
        r.read()


def stellarium_run_script(code):
    """
    Run a one-liner StelScript via POST /api/scripts/direct.
    Returns the raw response text (script output via core.output()).
    """
    body = urlencode({"code": code}).encode()
    req = Request(
        f"{STELLARIUM_URL}/api/scripts/direct", data=body, method="POST"
    )
    with urlopen(req, timeout=10) as r:
        return r.read().decode("utf-8").strip()


def stellarium_get_deltaT_algorithm():
    """Return the currently active DeltaT algorithm name."""
    return stellarium_run_script("core.output(core.getDeltaTAlgorithm());")


def stellarium_set_deltaT_algorithm(name):
    """Set the DeltaT algorithm by name (e.g. 'EspenakMeeus')."""
    stellarium_run_script(f'core.setDeltaTAlgorithm("{name}");')


def stellarium_set_time(jd):
    """Set Stellarium's simulation time to the given Julian Date."""
    url = f"{STELLARIUM_URL}/api/main/time"
    body = urlencode({"time": f"{jd:.6f}", "timerate": "0"}).encode()
    req = Request(url, data=body, method="POST")
    with urlopen(req, timeout=10) as r:
        r.read()


def stellarium_get_radec(name):
    """
    Return RA/Dec (J2000, degrees) for a named object from Stellarium.
    Stellarium reports raJ2000 / decJ2000 in radians.
    """
    params = urlencode({"name": name, "format": "json"})
    url = f"{STELLARIUM_URL}/api/objects/info?{params}"
    with urlopen(url, timeout=10) as r:
        data = json.loads(r.read().decode("utf-8"))

    if "raJ2000" not in data:
        raise KeyError(
            f"Stellarium does not know object '{name}'. "
            f"Response keys: {list(data.keys())}"
        )

    ra_raw = float(data["raJ2000"])
    dec_raw = float(data["decJ2000"])

    return ra_raw, dec_raw


# -- JPL Horizons OBSERVER ----------------------------------------------------


def fetch_jpl_observer(name, jd_start, jd_stop, step_days=7):
    """
    Fetch RA/Dec (ICRF, degrees) from JPL Horizons OBSERVER.

    observer_location: geocentric

    Returns: list of (jd, ra_deg, dec_deg)
    """
    cmd = OBJECT_IDS[name.lower()]

    params = {
        "format": "json",
        "MAKE_EPHEM": "'YES'",
        "COMMAND": f"'{cmd}'",
        "EPHEM_TYPE": "'OBSERVER'",
        "CENTER": "'500@399'",       # geocentric earth
        "START_TIME": f"'{jd_to_date_str(jd_start)} TT'",
        "STOP_TIME": f"'{jd_to_date_str(jd_stop)}'",
        "STEP_SIZE": f"'{step_days} d'",
        "QUANTITIES": "'1'",         # RA/Dec (ICRF)
        "ANG_FORMAT": "'DEG'",
        "CSV_FORMAT": "'YES'",
        "CAL_FORMAT": "'BOTH'",
        "TIME_DIGITS": "'MINUTES'",
        "EXTRA_PREC": "'YES'",
    }

    url = "https://ssd.jpl.nasa.gov/api/horizons.api?" + urlencode(params)
    with urlopen(url, timeout=120) as r:
        data = json.loads(r.read().decode("utf-8"))

    text = data.get("result", "")
    if "$$SOE" not in text:
        raise RuntimeError(
            f"No observer data from Horizons for '{name}':\n{text[:800]}"
        )

    block = text.split("$$SOE", 1)[1].split("$$EOE", 1)[0].strip()
    results = []

    for line in block.splitlines():
        line = line.strip()
        if not line:
            continue
        try:
            row = next(csv.reader([line], skipinitialspace=True))
            # CSV columns (OBSERVER, QUANTITIES=1, ANG_FORMAT=DEG,
            # CAL_FORMAT=BOTH):
            # Date__(UT)__HR:MN, Date_________JDUT, , ,
            # R.A.___(ICRF), DEC____(ICRF)
            jd = float(row[1])
            ra_deg = float(row[4])
            dec_deg = float(row[5])
            results.append((jd, ra_deg, dec_deg))
        except (ValueError, IndexError, StopIteration):
            continue  # skip header lines or empty rows

    if not results:
        raise RuntimeError(f"JPL returned empty data for '{name}'.")

    return results


# -- Angular distance ---------------------------------------------------------


def angular_distance_arcmin(ra1_deg, dec1_deg, ra2_deg, dec2_deg):
    """Compute angular separation between two RA/Dec positions in arcmin."""
    ra1 = math.radians(ra1_deg)
    dec1 = math.radians(dec1_deg)
    ra2 = math.radians(ra2_deg)
    dec2 = math.radians(dec2_deg)
    cos_d = (
        math.sin(dec1) * math.sin(dec2)
        + math.cos(dec1) * math.cos(dec2) * math.cos(ra1 - ra2)
    )
    cos_d = max(-1.0, min(1.0, cos_d))
    return math.degrees(math.acos(cos_d)) * 60.0


# -- Per-object comparison ----------------------------------------------------


def compare_object(name, jd_start, jd_stop, step_days=7, verbose=True):
    """
    Compare JPL OBSERVER vs Stellarium for one object.

    Returns (dates: list[datetime], errors_arcmin: list[float]),
    or ([], []) on error.
    """
    def log(msg):
        if verbose:
            print(msg, flush=True)

    log(f"\n-- {name} --")
    log("  Fetching JPL data ...")

    try:
        jpl_data = fetch_jpl_observer(name, jd_start, jd_stop, step_days)
    except Exception as e:
        print(f"  ERROR (JPL) for {name}: {e}", file=sys.stderr)
        return [], []

    log(f"  {len(jpl_data)} time steps received.")
    log("  Querying Stellarium ...")

    dates = []
    errors = []
    n = len(jpl_data)

    for i, (jd, ra_jpl, dec_jpl) in enumerate(jpl_data):
        try:
            stellarium_set_time(jd)
            time.sleep(STELLARIUM_DELAY)
            ra_stel, dec_stel = stellarium_get_radec(name)
        except KeyError as e:
            print(f"\n  ERROR (Stellarium): {e}", file=sys.stderr)
            return [], []
        except Exception as e:
            print(
                f"\n  WARNING: Stellarium query failed at JD {jd:.1f}: {e}",
                file=sys.stderr,
            )
            continue

        err = angular_distance_arcmin(ra_jpl, dec_jpl, ra_stel, dec_stel)
        dates.append(jd_to_datetime(jd))
        errors.append(err)

        if verbose and (i + 1) % 20 == 0:
            log(f"  {i+1}/{n} ({100*(i+1)//n} %)")

    log(f"  Done. {len(errors)} points compared.")
    return dates, errors


# -- Plot ---------------------------------------------------------------------


def make_plot(ax, name, dates, errors):
    """Draw the error curve for one object onto the given Axes."""
    if not dates:
        ax.text(
            0.5, 0.5, "No data",
            ha="center", va="center",
            transform=ax.transAxes, fontsize=12, color="gray",
        )
        ax.set_title(name)
        return

    mean_err = sum(errors) / len(errors)
    max_err = max(errors)
    max_idx = errors.index(max_err)

    ax.plot(dates, errors, linewidth=1.0, color="#2a6ebb")
    ax.axhline(
        mean_err, linestyle="--", linewidth=0.8, color="#cc4444",
        label=f"Mean: {mean_err:.3f}'",
    )

    ax.set_title(name, fontsize=13, fontweight="bold")
    ax.set_ylabel("Angular error (arcminutes)")
    ax.set_xlabel("Date (UTC)")
    ax.xaxis.set_major_formatter(mdates.DateFormatter("%Y-%m"))
    ax.xaxis.set_major_locator(mdates.MonthLocator(interval=3))
    plt.setp(ax.xaxis.get_majorticklabels(), rotation=30, ha="right")
    ax.yaxis.set_minor_locator(matplotlib.ticker.AutoMinorLocator())
    ax.grid(True, which="major", linestyle="--", alpha=0.5)
    ax.grid(True, which="minor", linestyle=":", alpha=0.25)
    ax.legend(fontsize=9)
    ax.annotate(
        f"Max: {max_err:.3f}'",
        xy=(dates[max_idx], max_err),
        xytext=(10, 5), textcoords="offset points",
        fontsize=8, color="#884400",
        arrowprops=dict(arrowstyle="->", color="#884400", lw=0.8),
    )


# -- Argument parser ----------------------------------------------------------


def parse_args():
    ap = argparse.ArgumentParser(
        description=(
            "Compare Stellarium moon and planet positions with JPL Horizons "
            "OBSERVER data and write a PDF with one error plot per object."
        ),
        epilog=(
            "Examples:\n"
            "  python3 %(prog)s ../data/ssystem_major.ini\n"
            "  python3 %(prog)s ../data/ssystem_major.ini himalia\n"
            "  python3 %(prog)s ../data/ssystem_major.ini "
            "--years 1 --out compare.pdf"
        ),
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    ap.add_argument("file", help="Path to ssystem_major.ini")
    ap.add_argument(
        "object", nargs="?",
        help="Optional: compare only this named object",
    )
    ap.add_argument(
        "--years", type=float, default=2.0,
        help="Time range in years (default: 2)",
    )
    ap.add_argument(
        "--step", type=int, default=7,
        help="Step size in days (default: 7)",
    )
    ap.add_argument(
        "--out", default="ssystem_compare.pdf",
        help="Output PDF filename (default: ssystem_compare.pdf)",
    )
    ap.add_argument(
        "--stellarium", default=STELLARIUM_URL,
        help=f"Stellarium base URL (default: {STELLARIUM_URL})",
    )
    ap.add_argument(
        "--delay", type=float, default=STELLARIUM_DELAY,
        help=(
            f"Seconds to wait after setting Stellarium time "
            f"(default: {STELLARIUM_DELAY})"
        ),
    )
    ap.add_argument(
        "--max-error", type=float, default=None, metavar="ARCMIN",
        help=(
            "Warn if the maximum angular error for an object exceeds "
            "this value in arcminutes (default: no limit)"
        ),
    )
    return ap.parse_args()


# -- Main ---------------------------------------------------------------------


def main():
    args = parse_args()

    global STELLARIUM_URL, STELLARIUM_DELAY
    STELLARIUM_URL = args.stellarium
    STELLARIUM_DELAY = args.delay

    # Check Stellarium connectivity
    print("Checking Stellarium connection ...")
    try:
        stellarium_check()
        print(f"  OK - {STELLARIUM_URL}")
    except Exception as e:
        print(
            f"ERROR: Stellarium not reachable ({e})\n"
            f"  Please start Stellarium and enable Remote Control.",
            file=sys.stderr,
        )
        sys.exit(1)

    # Save current Stellarium time to restore it afterwards
    original_jd = stellarium_get_time()

    # -- Save and configure Stellarium settings for comparison ----------------
    # All properties are saved, set to the required value, and restored.

    # (1) Aberration -- disable; JPL OBSERVER does not apply aberration
    #     correction.
    ABERRATION_PROP = "StelCore.flagUseAberration"
    original_aberration = stellarium_get_property(ABERRATION_PROP)
    stellarium_set_property(ABERRATION_PROP, "false")
    if stellarium_get_property(ABERRATION_PROP) is not False:
        print(
            f"  WARNING: Failed to set {ABERRATION_PROP} to false. "
            f"Positions may include aberration and not match JPL's "
            f"non-aberrated observer.",
            file=sys.stderr,
        )
    print(f"  {ABERRATION_PROP}: {original_aberration} -> false")

    # (2) Apparent coordinates equinox of date -- enable for comparison with
    #     JPL apparent RA/Dec (equinox of date). Stellarium property for the
    #     coordinate frame: "Equatorial" = equinox of date,
    #     "EquatorialJ2000" = J2000.
    FRAME_PROP = "StelCore.currentFrameType"
    original_frame = stellarium_get_property(FRAME_PROP)
    stellarium_set_property(FRAME_PROP, "Equatorial")
    print(f"  {FRAME_PROP}: {original_frame} -> Equatorial")

    # (3) TT timescale -- "Without correction" matches JPL's TDB/TT timescale.
    #     Stellarium property values: "None" = no correction (TT),
    #     "ELP2000-82B", etc.
    TIMECORR_PROP = "StelCore.currentTimeCorrectionAlgorithm"
    original_timecorr = stellarium_get_property(TIMECORR_PROP)
    stellarium_set_property(TIMECORR_PROP, "None")
    print(f"  {TIMECORR_PROP}: {original_timecorr} -> None")

    # (4) DE440 ephemeris -- must match the JPL source ephemeris.
    #     Stellarium property values: "DE440", "DE421", "DE406", etc.
    #     DE440 must be installed by the user.
    # EPHEM_PROP = "SolarSystem.currentEphemerisMethod"
    # original_ephem = stellarium_get_property(EPHEM_PROP)
    # stellarium_set_property(EPHEM_PROP, "DE440")
    # print(f"  {EPHEM_PROP}: {original_ephem} -> DE440")

    # (5) DeltaT algorithm -- set via StelScript API (no stelproperty
    #     equivalent). "EspenakMeeus" matches JPL's default delta-T model.
    original_deltat = stellarium_get_deltaT_algorithm()
    stellarium_set_deltaT_algorithm("None")
    print(f"  DeltaT algorithm: {original_deltat} -> None")

    # (6) Topocentric coordinates -- disable so positions are geocentric,
    #     matching the JPL observer at the surface point (coord@399).
    TOPOCENTRIC_PROP = "StelCore.flagUseTopocentricCoordinates"
    original_topocentric = stellarium_get_property(TOPOCENTRIC_PROP)
    stellarium_set_property(TOPOCENTRIC_PROP, "false")
    if stellarium_get_property(TOPOCENTRIC_PROP) is not False:
        print(
            f"  WARNING: Failed to set {TOPOCENTRIC_PROP} to false. "
            f"Positions may be topocentric and not match JPL's geocentric "
            f"observer.",
            file=sys.stderr,
        )
    print(f"  {TOPOCENTRIC_PROP}: {original_topocentric} -> false")

    # Read INI file
    with open(args.file, "r", encoding="utf-8") as f:
        lines = f.readlines()

    objects = get_objects(lines, args.object)

    if not objects:
        if args.object:
            print(
                f"ERROR: Object '{args.object}' not found in {args.file} "
                f"or not a valid moon or planet.",
                file=sys.stderr,
            )
        else:
            print(
                "ERROR: No comparable moon or planet objects found in the "
                "INI file.",
                file=sys.stderr,
            )
        sys.exit(1)

    jd_start = julian_date_now()
    jd_stop = jd_start + args.years * 365.25

    print(
        f"\nTime range : {jd_to_date_str(jd_start)} to "
        f"{jd_to_date_str(jd_stop)}"
    )
    print(f"Step size  : {args.step} days  |  Objects: {len(objects)}")
    print(f"Output     : {args.out}\n")

    # Run comparisons
    all_results = {}
    for obj in objects:
        name = obj["name"]
        dates, errors = compare_object(name, jd_start, jd_stop, args.step)
        all_results[name] = (dates, errors)

        if errors and args.max_error is not None:
            peak = max(errors)
            if peak > args.max_error:
                peak_date = dates[errors.index(peak)].strftime("%Y-%m-%d")
                print(
                    f"  WARNING: {name} exceeds max error threshold - "
                    f"peak {peak:.3f}' > {args.max_error:.3f}' "
                    f"(on {peak_date})",
                    file=sys.stderr,
                )

    # -- Restore all Stellarium settings --------------------------------------
    try:
        stellarium_set_time(original_jd)
        stellarium_set_property(ABERRATION_PROP, original_aberration)
        stellarium_set_property(FRAME_PROP, original_frame)
        stellarium_set_property(TIMECORR_PROP, original_timecorr)
        # stellarium_set_property(EPHEM_PROP, original_ephem)
        stellarium_set_deltaT_algorithm(original_deltat)
        stellarium_set_property(TOPOCENTRIC_PROP, original_topocentric)
        print(f"\nStellarium time restored (JD {original_jd:.4f}).")
        print(f"  {ABERRATION_PROP} -> {original_aberration}")
        print(f"  {FRAME_PROP} -> {original_frame}")
        print(f"  {TIMECORR_PROP} -> {original_timecorr}")
        # print(f"  {EPHEM_PROP} -> {original_ephem}")
        print(f"  DeltaT algorithm -> {original_deltat}")
        print(f"  {TOPOCENTRIC_PROP} -> {original_topocentric}")
    except Exception:
        pass

    # Build PDF
    print(f"\nWriting PDF '{args.out}' ...")
    observer_str = "geocentric observer"

    with PdfPages(args.out) as pdf:

        # Title page
        fig_t = plt.figure(figsize=(11, 4))
        fig_t.text(
            0.5, 0.68,
            "Stellarium vs. JPL Horizons OBSERVER",
            ha="center", va="center", fontsize=20, fontweight="bold",
        )
        fig_t.text(
            0.5, 0.44,
            f"Range: {jd_to_date_str(jd_start)} - "
            f"{jd_to_date_str(jd_stop)}"
            f"    Step: {args.step} days\n"
            f"Observer: {observer_str}\n"
            f"Coordinates: RA/Dec ICRF J2000    "
            f"Generated: "
            f"{datetime.now(timezone.utc).strftime('%Y-%m-%d %H:%M UTC')}",
            ha="center", va="center", fontsize=11, color="#444444",
        )
        pdf.savefig(fig_t, bbox_inches="tight")
        plt.close(fig_t)

        # One page per object
        for name, (dates, errors) in all_results.items():
            fig, ax = plt.subplots(figsize=(11, 5))
            make_plot(ax, name, dates, errors)
            fig.tight_layout()
            pdf.savefig(fig, bbox_inches="tight")
            plt.close(fig)

        # Summary page: all objects overlaid
        successful = {k: v for k, v in all_results.items() if v[1]}
        if len(successful) > 1:
            fig_s, ax_s = plt.subplots(figsize=(13, 6))
            cmap = matplotlib.colormaps.get_cmap("tab20")
            for idx, (name, (dates, errors)) in enumerate(
                successful.items()
            ):
                ax_s.plot(
                    dates, errors, linewidth=0.8,
                    label=name, color=cmap(idx % 20),
                )
            ax_s.set_title(
                "Overview: All Objects", fontsize=13, fontweight="bold"
            )
            ax_s.set_ylabel("Angular error (arcminutes)")
            ax_s.set_xlabel("Date (UTC)")
            ax_s.xaxis.set_major_formatter(mdates.DateFormatter("%Y-%m"))
            ax_s.xaxis.set_major_locator(mdates.MonthLocator(interval=3))
            plt.setp(
                ax_s.xaxis.get_majorticklabels(), rotation=30, ha="right"
            )
            ax_s.grid(True, linestyle="--", alpha=0.4)
            ax_s.legend(fontsize=7, ncol=4, loc="upper right")
            fig_s.tight_layout()
            pdf.savefig(fig_s, bbox_inches="tight")
            plt.close(fig_s)

        # Summary table page
        stats = [
            (name, sum(errs) / len(errs), max(errs))
            for name, (_, errs) in all_results.items() if errs
        ]
        if stats:
            stats.sort(key=lambda r: r[2], reverse=True)  # sort by max desc
            col_labels = ["Object", "Mean error (')", "Max error (')"]
            table_data = [
                [name, f"{mean:.4f}", f"{peak:.4f}"]
                for name, mean, peak in stats
            ]

            # One table row is ~0.022 fig-height; fit ~40 rows per page.
            rows_per_page = 40
            for page_start in range(0, len(table_data), rows_per_page):
                chunk = table_data[page_start:page_start + rows_per_page]
                fig_tb, ax_tb = plt.subplots(
                    figsize=(9, max(3, len(chunk) * 0.32 + 1.2))
                )
                ax_tb.axis("off")
                tbl = ax_tb.table(
                    cellText=chunk,
                    colLabels=col_labels,
                    cellLoc="center",
                    loc="center",
                )
                tbl.auto_set_font_size(False)
                tbl.set_fontsize(9)
                tbl.auto_set_column_width([0, 1, 2])
                # Style header row
                for col in range(3):
                    tbl[(0, col)].set_facecolor("#2a6ebb")
                    tbl[(0, col)].set_text_props(
                        color="white", fontweight="bold"
                    )
                # Highlight rows that exceed max_error threshold
                if args.max_error is not None:
                    for row_idx, (name, mean, peak) in enumerate(
                        stats[page_start:page_start + rows_per_page],
                        start=1,
                    ):
                        if peak > args.max_error:
                            for col in range(3):
                                tbl[(row_idx, col)].set_facecolor("#ffe0e0")
                page_label = (
                    f" ({page_start // rows_per_page + 1})"
                    if len(table_data) > rows_per_page else ""
                )
                title = (
                    f"Summary: Mean & Max Error per Object{page_label}"
                )
                if args.max_error is not None:
                    title += f"  --  red: exceeds {args.max_error:.3f}'"
                ax_tb.set_title(
                    title, fontsize=11, fontweight="bold", pad=12
                )
                fig_tb.tight_layout()
                pdf.savefig(fig_tb, bbox_inches="tight")
                plt.close(fig_tb)

        meta = pdf.infodict()
        meta["Title"] = "Stellarium vs JPL Horizons Comparison"
        meta["Author"] = "compare_ssystem.py"
        meta["Subject"] = "Moon ephemeris angular error"

    n_pages = (
        1 + len(all_results)
        + (1 if len(successful) > 1 else 0)
        + (1 if stats else 0)
    )
    print(f"Done. PDF saved: {args.out}  ({n_pages} pages)")

    # CSV summary to stdout
    if stats:
        print("\nObject,Mean error (arcmin),Max error (arcmin)")
        for name, mean, peak in stats:
            print(f"{name},{mean:.4f},{peak:.4f}")


if __name__ == "__main__":
    main()
