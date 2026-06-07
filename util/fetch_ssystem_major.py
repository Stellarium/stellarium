#!/usr/bin/env python3
"""
fetch_ssystem_major.py -- Update Stellarium moon orbital elements via JPL
Horizons.

Reads ssystem_major.ini, fetches current osculating elements for each known
moon from JPL Horizons (ELEMENTS ephemeris type), and writes the updated INI
back to stdout. Check results carefully before replacing the original file.

Usage:
    python3 fetch_ssystem_major.py ../data/ssystem_major.ini
    python3 fetch_ssystem_major.py ../data/ssystem_major.ini himalia
"""

import argparse
import csv
import json
import re
import sys
from datetime import datetime, timezone
from urllib.parse import urlencode
from urllib.request import urlopen

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

ORBIT_KEYS = {
    "orbit_AscendingNode",
    "orbit_Eccentricity",
    "orbit_Epoch",
    "orbit_Inclination",
    "orbit_LongOfPericenter",
    "orbit_MeanLongitude",
    "orbit_Period",
    "orbit_SemiMajorAxis",
}

# Mapping for case-insensitive lookup
ORBIT_KEYS_MAP = {k.lower(): k for k in ORBIT_KEYS}


def julian_date_now():
    """Return today's date as Julian Date at 0:00 UTC."""
    dt = datetime.now(timezone.utc)
    y, m = dt.year, dt.month
    d = dt.day

    if m <= 2:
        y -= 1
        m += 12

    a = y // 100
    b = 2 - a + a // 4

    # Result is always *.5 (midnight UTC)
    jd = (
        int(365.25 * (y + 4716))
        + int(30.6001 * (m + 1))
        + d + b - 1524.5
    )
    return jd


def norm_angle(x):
    """Normalise an angle to the range [0, 360)."""
    return x % 360.0


def fetch_elements(name, parent, jd):
    """
    Fetch osculating orbital elements for *name* from JPL Horizons.

    The query uses EPHEM_TYPE=ELEMENTS relative to the body-centred frame
    of *parent*. Returns a dict of Stellarium orbit_* key-value pairs.
    """
    cmd = OBJECT_IDS[name.lower()]
    center = "500@" + PARENT_IDS[parent]

    params = {
        "format": "json",
        "MAKE_EPHEM": "YES",
        "COMMAND": cmd,
        "EPHEM_TYPE": "ELEMENTS",
        "CENTER": center,
        "TLIST_TYPE": "JD",
        "TIME_TYPE": "TDB",
        "TLIST": str(jd),
        "REF_PLANE": "BODY",
        "CSV_FORMAT": "YES",
    }

    url = "https://ssd.jpl.nasa.gov/api/horizons.api?" + urlencode(params)

    with urlopen(url, timeout=60) as r:
        data = json.loads(r.read().decode("utf-8"))

    text = data["result"]

    if "$$SOE" not in text:
        raise RuntimeError(
            f"No element data from Horizons for '{name}':\n{text[:1000]}"
        )

    block = text.split("$$SOE", 1)[1].split("$$EOE", 1)[0].strip()
    row = next(csv.reader([block.splitlines()[0]], skipinitialspace=True))

    # CSV columns for ELEMENTS:
    # JDTDB, Calendar Date, EC, QR, IN, OM, W, Tp, N, MA, TA, A, AD, PR
    ec = float(row[2])
    inc = float(row[4])
    om = float(row[5])
    w = float(row[6])
    ma = float(row[9])
    a = float(row[11])

    pr_seconds = float(row[13])
    pr = pr_seconds / 86400.0  # convert seconds to days

    long_peri = norm_angle(om + w)
    mean_long = norm_angle(om + w + ma)

    return {
        "orbit_AscendingNode": om,
        "orbit_Eccentricity": ec,
        "orbit_Epoch": jd,
        "orbit_Inclination": inc,
        "orbit_LongOfPericenter": long_peri,
        "orbit_MeanLongitude": mean_long,
        "orbit_Period": pr,
        "orbit_SemiMajorAxis": a,
    }


def split_value_line(line):
    """
    Parse a key = value line from the INI file.

    Returns (prefix, key, value, suffix) or None if the line does not match.
    """
    m = re.match(r"^(\s*([^=\s#]+)\s*=\s*)(.*?)(\s*(?:#.*)?\n?)$", line)
    if not m:
        return None
    return m.group(1), m.group(2), m.group(3), m.group(4)


def format_value(key, val):
    """Return *val* formatted as a string appropriate for *key*."""
    key = key.lower()

    if key == "orbit_epoch":
        return f"{val:.1f}"

    if key == "orbit_eccentricity":
        return f"{val:.16g}"

    return f"{val:.15g}"


def read_sections(lines):
    """
    Parse *lines* into a list of section dicts.

    Each dict contains: name, start (line index), end (line index),
    keys (dict mapping key name to line index).
    """
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


def parse_args():
    """Parse and return command-line arguments."""
    example_text = (
        "example:\n\n"
        "  python3 %(prog)s ../data/ssystem_major.ini himalia"
    )

    ap = argparse.ArgumentParser(
        description=(
            "Updates Stellarium moon orbits via JPL Horizons. "
            "Check results carefully."
        ),
        epilog=example_text,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    ap.add_argument("file", help="ssystem_major.ini file")
    ap.add_argument(
        "object", nargs="?",
        help="Optional: update only this named object",
    )

    try:
        args = ap.parse_args()
    except SystemExit:
        ap.print_help()
        sys.exit(0)

    return args


def warn(message):
    """Print a warning message to stderr."""
    print(f"WARN: {message}", file=sys.stderr)


def get_section_lines(lines, sec):
    """Return the raw lines belonging to *sec*."""
    return lines[sec["start"]:sec["end"]]


def should_skip_section(sec_name, requested_object):
    """Return True if this section should be skipped for the given filter."""
    return requested_object and sec_name.lower() != requested_object.lower()


def validate_moon_section(sec, lines):
    """
    Validate that *sec* describes a moon with a known parent and object ID.

    Returns an error string if validation fails, or None on success.
    """
    sec_name = sec["name"]
    keys = sec["keys"]

    if "type" not in keys:
        return f"type key for {sec_name} missing"

    type_line = lines[keys["type"]]
    parts = split_value_line(type_line)
    if not parts or parts[2].strip() != "moon":
        return f"ignoring object {sec_name}, not a moon"

    if "parent" not in keys:
        return f"parent key for {sec_name} missing"

    parent = split_value_line(lines[keys["parent"]])[2].strip()
    if parent not in PARENT_IDS:
        return f"parent ID for {parent} in {sec_name} unknown"

    if sec_name.lower() not in OBJECT_IDS:
        return f"ID for {sec_name} unknown"

    return None


def get_parent(sec, lines):
    """Return the parent body name for *sec*."""
    return split_value_line(lines[sec["keys"]["parent"]])[2].strip()


def replace_key_in_prefix(prefix, canonical_key):
    """
    Replace the key name in an INI prefix string with *canonical_key*.

    Preserves the original leading whitespace. The prefix is the part of
    an INI line up to and including the '=' sign.
    """
    if "=" not in prefix:
        return prefix

    key_part, sep, rest = prefix.partition("=")

    # Preserve original indentation/whitespace before the key
    leading = key_part[: len(key_part) - len(key_part.lstrip())]

    return leading + canonical_key + sep + rest


def update_orbit_lines(lines, sec, vals):
    """
    Update the orbit_* lines in *lines* for *sec* with values from *vals*.

    Keys are matched case-insensitively; only recognised orbit keys are
    updated.
    """
    keys = sec["keys"]

    print(
        f" ** Updating orbit lines for {sec['name']} "
        f"with keys {keys} and values {vals}",
        file=sys.stderr,
    )

    for key, idx in keys.items():
        # Perform case-insensitive lookup
        canonical_key = ORBIT_KEYS_MAP.get(key.lower())

        # Skip keys that are not orbit-related
        if not canonical_key:
            continue

        parts = split_value_line(lines[idx])
        if not parts:
            continue

        prefix, _, _, suffix = parts

        # Replace key name with canonical capitalisation
        prefix = replace_key_in_prefix(prefix, canonical_key)

        lines[idx] = (
            prefix
            + format_value(canonical_key, vals[canonical_key])
            + suffix
        )


def process_section(sec, lines, jd, requested_object):
    """
    Fetch and apply updated elements for one INI section.

    Returns (matched: bool, output_lines: list).
    """
    sec_name = sec["name"]

    if should_skip_section(sec_name, requested_object):
        return False, []

    error = validate_moon_section(sec, lines)
    if error:
        warn(error)
        if requested_object:
            return True, get_section_lines(lines, sec)
        return False, []

    parent = get_parent(sec, lines)
    vals = fetch_elements(sec_name, parent, jd)
    update_orbit_lines(lines, sec, vals)

    return True, get_section_lines(lines, sec)


def main():
    """Entry point: parse arguments, process all sections, write output."""
    args = parse_args()

    with open(args.file, "r", encoding="utf-8") as f:
        lines = f.readlines()

    jd = julian_date_now()
    sections = read_sections(lines)
    output_lines = []

    for sec in sections:
        matched, sec_output = process_section(sec, lines, jd, args.object)

        if args.object:
            if matched:
                output_lines.extend(sec_output)
                break
            continue

    if not args.object:
        output_lines = lines

    sys.stdout.write("".join(output_lines))


if __name__ == "__main__":
    main()
