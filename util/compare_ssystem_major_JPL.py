#!/usr/bin/env python3
"""
compare_ssystem.py – Compare Stellarium object positions with JPL Horizons OBSERVER data.

For each moon in ssystem_major.ini, RA/Dec positions are fetched from JPL Horizons
and from the locally running Stellarium instance over a configurable time range
(default: 2 years, weekly). The angular error in arcminutes is plotted per object
and saved as a multi-page PDF.

Observer location: read from Stellarium at startup and passed to JPL as geodetic
surface coordinates, so both sides use exactly the same observer.

Usage:
    python3 compare_ssystem.py ../data/ssystem_major.ini
    python3 compare_ssystem.py ../data/ssystem_major.ini himalia
    python3 compare_ssystem.py ../data/ssystem_major.ini --years 1 --out my_compare.pdf
"""

import sys, re, json, csv, argparse, math, time
from datetime import datetime, timezone
from urllib.parse import urlencode
from urllib.request import urlopen, Request
from urllib.error import URLError

try:
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    import matplotlib.dates as mdates
    import matplotlib.ticker
    from matplotlib.backends.backend_pdf import PdfPages
except ImportError:
    print("ERROR: matplotlib not found. Install with: pip install matplotlib", file=sys.stderr)
    sys.exit(1)

# ── Configuration ──────────────────────────────────────────────────────────────

STELLARIUM_URL = "http://localhost:8090"

# Seconds to wait after setting Stellarium time before querying position.
# Increase on slow machines if positions appear stale.
STELLARIUM_DELAY = 0.05

# ── JPL Horizons object IDs ────────────────────────────────────────────────────

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
    "Mercury": "199", "Venus": "299", "Earth": "399", "Mars": "499",
    "Jupiter": "599", "Saturn": "699", "Uranus": "799", "Neptune": "899",
    "Pluto": "999",
}

# ── Time utilities ─────────────────────────────────────────────────────────────

def julian_date_now():
    """Return today's date as Julian Date at 0:00 UTC."""
    dt = datetime.now(timezone.utc)
    y, m, d = dt.year, dt.month, dt.day
    if m <= 2:
        y -= 1
        m += 12
    a = y // 100
    b = 2 - a + a // 4
    return int(365.25 * (y + 4716)) + int(30.6001 * (m + 1)) + d + b - 1524.5


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
    day   = b - d_int - int(30.6001 * e)
    month = e - 1 if e < 14 else e - 13
    year  = c - 4716 if month > 2 else c - 4715
    fh = f * 24
    h  = int(fh)
    fm = (fh - h) * 60
    m_ = int(fm)
    s  = int((fm - m_) * 60)
    return datetime(year, month, day, h, m_, s, tzinfo=timezone.utc)


def jd_to_date_str(jd):
    """Format Julian Date as 'YYYY-MM-DD' for JPL API calls."""
    return jd_to_datetime(jd).strftime("%Y-%m-%d")

# ── INI parser (shared logic with fetch_ssystem_major.py) ─────────────────────

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
            current = {"name": m.group(1), "start": i, "end": len(lines), "keys": {}}
            sections.append(current)
            if len(sections) > 1:
                sections[-2]["end"] = i
        if current:
            p = split_value_line(line)
            if p:
                current["keys"][p[1]] = i
    return sections


def get_moon_sections(lines, requested_object=None):
    """
    Return all sections with type=moon, a known parent, and a known OBJECT_ID.
    If requested_object is given, return only that one section.
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
        if not type_val or type_val[2].strip() != "moon":
            continue
        if "parent" not in keys:
            continue
        parent = split_value_line(lines[keys["parent"]])[2].strip()
        if parent not in PARENT_IDS:
            continue
        if name.lower() not in OBJECT_IDS:
            continue

        result.append({"name": name, "parent": parent})

    return result

# ── Stellarium API ─────────────────────────────────────────────────────────────

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
    Reads from GET /api/main/status → time.jday
    (GET /api/main/time is not supported; only POST for setting.)
    """
    data = stellarium_status()
    time_block = data.get("time", {})
    if isinstance(time_block, dict):
        return float(time_block["jday"])
    return float(data["jday"])  # fallback for older Stellarium versions


def stellarium_get_location():
    """
    Return the observer location configured in Stellarium as
    (longitude_deg, latitude_deg, altitude_km).
    Reads from GET /api/main/status → location.
    """
    data = stellarium_status()
    loc  = data.get("location", {})
    lon  = float(loc.get("longitude", 0.0))
    lat  = float(loc.get("latitude",  0.0))
    alt  = float(loc.get("altitude",  0.0))  # Stellarium returns metres
    return lon, lat, alt / 1000.0             # convert altitude to km for JPL


def stellarium_set_time(jd):
    """Set Stellarium's simulation time to the given Julian Date."""
    url  = f"{STELLARIUM_URL}/api/main/time"
    body = urlencode({"time": f"{jd:.6f}", "timerate": "0"}).encode()
    req  = Request(url, data=body, method="POST")
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

    ra_raw  = float(data["raJ2000"])
    dec_raw = float(data["decJ2000"])

    # Values > 7 indicate degrees (older Stellarium), otherwise radians
    if ra_raw > 7.0:
        return ra_raw, dec_raw
    return math.degrees(ra_raw), math.degrees(dec_raw)

# ── JPL Horizons OBSERVER ──────────────────────────────────────────────────────

def fetch_jpl_observer(name, jd_start, jd_stop, observer_location, step_days=7):
    """
    Fetch RA/Dec (ICRF, degrees) from JPL Horizons OBSERVER.

    observer_location: (longitude_deg, latitude_deg, altitude_km)
        Uses geodetic surface coordinates (COORD_TYPE=GEODETIC, CENTER=coord@399)
        matching Stellarium's configured observer location exactly.

    Returns: list of (jd, ra_deg, dec_deg)
    """
    cmd = OBJECT_IDS[name.lower()]
    lon, lat, alt_km = observer_location

    params = {
        "format":      "json",
        "MAKE_EPHEM":  "'YES'",
        "COMMAND":     f"'{cmd}'",
        "EPHEM_TYPE":  "'OBSERVER'",
        "CENTER":      "'coord@399'",      # geodetic surface observer on Earth
        "COORD_TYPE":  "'GEODETIC'",
        "SITE_COORD":  f"'{lon:.6f},{lat:.6f},{alt_km:.4f}'",
        "START_TIME":  f"'{jd_to_date_str(jd_start)}'",
        "STOP_TIME":   f"'{jd_to_date_str(jd_stop)}'",
        "STEP_SIZE":   f"'{step_days} d'",
        "QUANTITIES":  "'1'",              # RA/Dec (ICRF)
        "ANG_FORMAT":  "'DEG'",
        "CSV_FORMAT":  "'YES'",
        "CAL_FORMAT":  "'BOTH'",
        "TIME_DIGITS": "'MINUTES'",
        "EXTRA_PREC":  "'YES'",
    }

    url = "https://ssd.jpl.nasa.gov/api/horizons.api?" + urlencode(params)
    with urlopen(url, timeout=120) as r:
        data = json.loads(r.read().decode("utf-8"))

    text = data.get("result", "")
    if "$$SOE" not in text:
        raise RuntimeError(
            f"No observer data from Horizons for '{name}':\n{text[:800]}"
        )

    block   = text.split("$$SOE", 1)[1].split("$$EOE", 1)[0].strip()
    results = []

    for line in block.splitlines():
        line = line.strip()
        if not line:
            continue
        try:
            row = next(csv.reader([line], skipinitialspace=True))
            # CSV columns (OBSERVER, QUANTITIES=1, ANG_FORMAT=DEG, CAL_FORMAT=BOTH):
            # Date__(UT)__HR:MN, Date_________JDUT, , , R.A.___(ICRF), DEC____(ICRF)
            jd      = float(row[1])
            ra_deg  = float(row[4])
            dec_deg = float(row[5])
            results.append((jd, ra_deg, dec_deg))
        except (ValueError, IndexError, StopIteration):
            continue  # skip header lines or empty rows

    if not results:
        raise RuntimeError(f"JPL returned empty data for '{name}'.")

    return results

# ── Angular distance ───────────────────────────────────────────────────────────

def angular_distance_arcmin(ra1_deg, dec1_deg, ra2_deg, dec2_deg):
    """Compute angular separation between two RA/Dec positions in arcminutes."""
    ra1  = math.radians(ra1_deg)
    dec1 = math.radians(dec1_deg)
    ra2  = math.radians(ra2_deg)
    dec2 = math.radians(dec2_deg)
    cos_d = (math.sin(dec1) * math.sin(dec2) +
             math.cos(dec1) * math.cos(dec2) * math.cos(ra1 - ra2))
    cos_d = max(-1.0, min(1.0, cos_d))
    return math.degrees(math.acos(cos_d)) * 60.0

# ── Per-object comparison ──────────────────────────────────────────────────────

def compare_object(name, jd_start, jd_stop, observer_location, step_days=7, verbose=True):
    """
    Compare JPL OBSERVER vs Stellarium for one object.

    Returns (dates: list[datetime], errors_arcmin: list[float]),
    or ([], []) on error.
    """
    def log(msg):
        if verbose:
            print(msg, flush=True)

    log(f"\n── {name} ──")
    log("  Fetching JPL data ...")

    try:
        jpl_data = fetch_jpl_observer(name, jd_start, jd_stop, observer_location, step_days)
    except Exception as e:
        print(f"  ERROR (JPL) for {name}: {e}", file=sys.stderr)
        return [], []

    log(f"  {len(jpl_data)} time steps received.")
    log("  Querying Stellarium ...")

    dates  = []
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
            print(f"\n  WARNING: Stellarium query failed at JD {jd:.1f}: {e}",
                  file=sys.stderr)
            continue

        err = angular_distance_arcmin(ra_jpl, dec_jpl, ra_stel, dec_stel)
        dates.append(jd_to_datetime(jd))
        errors.append(err)

        if verbose and (i + 1) % 20 == 0:
            log(f"  {i+1}/{n} ({100*(i+1)//n} %)")

    log(f"  Done. {len(errors)} points compared.")
    return dates, errors

# ── Plot ───────────────────────────────────────────────────────────────────────

def make_plot(ax, name, dates, errors):
    """Draw the error curve for one object onto the given Axes."""
    if not dates:
        ax.text(0.5, 0.5, "No data", ha="center", va="center",
                transform=ax.transAxes, fontsize=12, color="gray")
        ax.set_title(name)
        return

    mean_err = sum(errors) / len(errors)
    max_err  = max(errors)
    max_idx  = errors.index(max_err)

    ax.plot(dates, errors, linewidth=1.0, color="#2a6ebb")
    ax.axhline(mean_err, linestyle="--", linewidth=0.8, color="#cc4444",
               label=f"Mean: {mean_err:.3f}'")

    ax.set_title(name, fontsize=13, fontweight="bold")
    ax.set_ylabel("Angular error (arcminutes)")
    ax.set_xlabel("Date (UTC)")
    ax.xaxis.set_major_formatter(mdates.DateFormatter("%Y-%m"))
    ax.xaxis.set_major_locator(mdates.MonthLocator(interval=3))
    plt.setp(ax.xaxis.get_majorticklabels(), rotation=30, ha="right")
    ax.yaxis.set_minor_locator(matplotlib.ticker.AutoMinorLocator())
    ax.grid(True, which="major", linestyle="--", alpha=0.5)
    ax.grid(True, which="minor", linestyle=":",  alpha=0.25)
    ax.legend(fontsize=9)
    ax.annotate(
        f"Max: {max_err:.3f}'",
        xy=(dates[max_idx], max_err),
        xytext=(10, 5), textcoords="offset points",
        fontsize=8, color="#884400",
        arrowprops=dict(arrowstyle="->", color="#884400", lw=0.8),
    )

# ── Argument parser ────────────────────────────────────────────────────────────

def parse_args():
    ap = argparse.ArgumentParser(
        description=(
            "Compare Stellarium moon positions with JPL Horizons OBSERVER data "
            "and write a PDF with one error plot per object."
        ),
        epilog=(
            "Examples:\n"
            "  python3 %(prog)s ../data/ssystem_major.ini\n"
            "  python3 %(prog)s ../data/ssystem_major.ini himalia\n"
            "  python3 %(prog)s ../data/ssystem_major.ini --years 1 --out compare.pdf"
        ),
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    ap.add_argument("file",   help="Path to ssystem_major.ini")
    ap.add_argument("object", nargs="?",
                    help="Optional: compare only this named object")
    ap.add_argument("--years", type=float, default=2.0,
                    help="Time range in years (default: 2)")
    ap.add_argument("--step",  type=int,   default=7,
                    help="Step size in days (default: 7)")
    ap.add_argument("--out",   default="ssystem_compare.pdf",
                    help="Output PDF filename (default: ssystem_compare.pdf)")
    ap.add_argument("--stellarium", default=STELLARIUM_URL,
                    help=f"Stellarium base URL (default: {STELLARIUM_URL})")
    ap.add_argument("--delay", type=float, default=STELLARIUM_DELAY,
                    help=f"Seconds to wait after setting Stellarium time "
                         f"(default: {STELLARIUM_DELAY})")
    ap.add_argument("--max-error", type=float, default=None, metavar="ARCMIN",
                    help="Warn if the maximum angular error for an object exceeds "
                         "this value in arcminutes (default: no limit)")
    return ap.parse_args()

# ── Main ───────────────────────────────────────────────────────────────────────

def main():
    args = parse_args()

    global STELLARIUM_URL, STELLARIUM_DELAY
    STELLARIUM_URL   = args.stellarium
    STELLARIUM_DELAY = args.delay

    # Check Stellarium connectivity
    print("Checking Stellarium connection ...")
    try:
        stellarium_check()
        print(f"  OK – {STELLARIUM_URL}")
    except Exception as e:
        print(f"ERROR: Stellarium not reachable ({e})\n"
              f"  Please start Stellarium and enable Remote Control.",
              file=sys.stderr)
        sys.exit(1)

    # Save current Stellarium time to restore it afterwards
    original_jd = stellarium_get_time()

    # Read observer location from Stellarium and use it for JPL too
    observer_location = stellarium_get_location()
    lon, lat, alt_km = observer_location
    print(f"  Observer : lon={lon:.4f}°  lat={lat:.4f}°  alt={alt_km*1000:.0f} m")
    print(f"  (same coordinates passed to JPL Horizons)")

    # Read INI file
    with open(args.file, "r", encoding="utf-8") as f:
        lines = f.readlines()

    objects = get_moon_sections(lines, args.object)

    if not objects:
        if args.object:
            print(f"ERROR: Object '{args.object}' not found in {args.file} "
                  f"or not a valid moon.", file=sys.stderr)
        else:
            print("ERROR: No comparable moon objects found in the INI file.",
                  file=sys.stderr)
        sys.exit(1)

    jd_start = julian_date_now()
    jd_stop  = jd_start + args.years * 365.25

    print(f"\nTime range : {jd_to_date_str(jd_start)} to {jd_to_date_str(jd_stop)}")
    print(f"Step size  : {args.step} days  |  Objects: {len(objects)}")
    print(f"Output     : {args.out}\n")

    # Run comparisons
    all_results = {}
    for obj in objects:
        name = obj["name"]
        dates, errors = compare_object(
            name, jd_start, jd_stop, observer_location, args.step
        )
        all_results[name] = (dates, errors)

        if errors and args.max_error is not None:
            peak = max(errors)
            if peak > args.max_error:
                peak_date = dates[errors.index(peak)].strftime("%Y-%m-%d")
                print(f"  WARNING: {name} exceeds max error threshold – "
                      f"peak {peak:.3f}' > {args.max_error:.3f}' "
                      f"(on {peak_date})", file=sys.stderr)

    # Restore Stellarium time
    try:
        stellarium_set_time(original_jd)
        print(f"\nStellarium time restored (JD {original_jd:.4f}).")
    except Exception:
        pass

    # Build PDF
    print(f"\nWriting PDF '{args.out}' ...")
    observer_str = f"lon={lon:.4f}°  lat={lat:.4f}°  alt={alt_km*1000:.0f} m"

    with PdfPages(args.out) as pdf:

        # Title page
        fig_t = plt.figure(figsize=(11, 4))
        fig_t.text(0.5, 0.68, "Stellarium vs. JPL Horizons OBSERVER",
                   ha="center", va="center", fontsize=20, fontweight="bold")
        fig_t.text(
            0.5, 0.44,
            f"Range: {jd_to_date_str(jd_start)} – {jd_to_date_str(jd_stop)}"
            f"    Step: {args.step} days\n"
            f"Observer: {observer_str}\n"
            f"Coordinates: RA/Dec ICRF J2000    "
            f"Generated: {datetime.now(timezone.utc).strftime('%Y-%m-%d %H:%M UTC')}",
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
            for idx, (name, (dates, errors)) in enumerate(successful.items()):
                ax_s.plot(dates, errors, linewidth=0.8,
                          label=name, color=cmap(idx % 20))
            ax_s.set_title("Overview: All Objects", fontsize=13, fontweight="bold")
            ax_s.set_ylabel("Angular error (arcminutes)")
            ax_s.set_xlabel("Date (UTC)")
            ax_s.xaxis.set_major_formatter(mdates.DateFormatter("%Y-%m"))
            ax_s.xaxis.set_major_locator(mdates.MonthLocator(interval=3))
            plt.setp(ax_s.xaxis.get_majorticklabels(), rotation=30, ha="right")
            ax_s.grid(True, linestyle="--", alpha=0.4)
            ax_s.legend(fontsize=7, ncol=4, loc="upper right")
            fig_s.tight_layout()
            pdf.savefig(fig_s, bbox_inches="tight")
            plt.close(fig_s)

        meta = pdf.infodict()
        meta["Title"]   = "Stellarium vs JPL Horizons Comparison"
        meta["Author"]  = "compare_ssystem.py"
        meta["Subject"] = "Moon ephemeris angular error"

    n_pages = 1 + len(all_results) + (1 if len(successful) > 1 else 0)
    print(f"Done. PDF saved: {args.out}  ({n_pages} pages)")


if __name__ == "__main__":
    main()
