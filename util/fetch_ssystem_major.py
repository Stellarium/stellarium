#!/usr/bin/env python3
import sys, re, json, csv, argparse
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
    "Mercury": "199", "Venus": "299", "Earth": "399", "Mars": "499",
    "Jupiter": "599", "Saturn": "699", "Uranus": "799", "Neptune": "899",
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

def julian_date_now():
    dt = datetime.now(timezone.utc)

    y, m = dt.year, dt.month
    d = dt.day  # remove time

    if m <= 2:
        y -= 1
        m += 12

    a = y // 100
    b = 2 - a + a // 4

    jd = int(365.25 * (y + 4716)) + int(30.6001 * (m + 1)) + d + b - 1524.5

    return jd  # result is always *.5 (0:00 UTC)

def norm_angle(x):
    return x % 360.0

def fetch_elements(name, parent, jd):
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
        raise RuntimeError("Keine Elements-Daten von Horizons für %s:\n%s" % (name, text[:1000]))

    block = text.split("$$SOE", 1)[1].split("$$EOE", 1)[0].strip()
    row = next(csv.reader([block.splitlines()[0]], skipinitialspace=True))

    # CSV columns bei ELEMENTS:
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
    m = re.match(r"^(\s*([^=\s#]+)\s*=\s*)(.*?)(\s*(?:#.*)?\n?)$", line)
    if not m:
        return None
    return m.group(1), m.group(2), m.group(3), m.group(4)

def format_value(key, val):
    if key == "orbit_Epoch":
        return "%.1f" % val
    if key == "orbit_Eccentricity":
        return "%.16g" % val
    return "%.15g" % val

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

def parse_args():
    example_text = '''example:

    python3 %(prog)s ../data/ssystem_major.ini himalia'''

    ap = argparse.ArgumentParser(
        description="Updates Stellarium moon orbits via JPL Horizons. Check results carefully",
        epilog=example_text
    )
    ap.add_argument("file", help="ssystem_major.ini file")
    ap.add_argument("object", nargs="?", help="Optional: update only one named object")

    try:
        args = ap.parse_args()
    except SystemExit:
        ap.print_help()
        sys.exit(0)

    return args


def warn(message):
    print(f"WARN: {message}", file=sys.stderr)


def get_section_lines(lines, sec):
    return lines[sec["start"]:sec["end"]]


def should_skip_section(sec_name, requested_object):
    return requested_object and sec_name.lower() != requested_object.lower()


def validate_moon_section(sec, lines):
    sec_name = sec["name"]
    keys = sec["keys"]

    if "type" not in keys:
        return f"type object for {sec_name} missing"

    type_line = lines[keys["type"]]
    parts = split_value_line(type_line)
    if not parts or parts[2].strip() != "moon":
        return f"ignoring object {sec_name}, not a moon"

    if "parent" not in keys:
        return f"parent object for {sec_name} missing"

    parent = split_value_line(lines[keys["parent"]])[2].strip()
    if parent not in PARENT_IDS:
        return f"parent ID for {parent} in {sec_name} unknown"

    if sec_name.lower() not in OBJECT_IDS:
        return f"ID for {sec_name} unknown"

    return None


def get_parent(sec, lines):
    return split_value_line(lines[sec["keys"]["parent"]])[2].strip()


def update_orbit_lines(lines, sec, vals):
    keys = sec["keys"]

    for key in ORBIT_KEYS & set(keys):
        idx = keys[key]
        parts = split_value_line(lines[idx])
        if not parts:
            continue

        prefix, _, _, suffix = parts
        lines[idx] = prefix + format_value(key, vals[key]) + suffix


def process_section(sec, lines, jd, requested_object):
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
