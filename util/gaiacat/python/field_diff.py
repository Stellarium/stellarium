#!/usr/bin/env python3
"""Exhaustive star-by-star field comparison of two Star1 catalog sets
(official vs gaiahip2cat output), levels 0-3. Matches globally by gaia_id
(HIP<<5|comp when gaia_id==0); classifies every differing star by cause.
Output: a Markdown report with per-star tables (no differing star omitted)
and a TSV file for B-V only differences (method difference, not matching
criterion).
"""
import struct
import math
import os
import json
import re
from collections import defaultdict
import pandas as pd

OFFICIAL = r"C:\Users\13308\CLionProjects\stellarium\stars\hip_gaia3"
OURS = r"F:\gaiahip2cat_out"
SIMBAD = r"C:\Users\13308\CLionProjects\stellarium_star_catalogs\simbad_query_results\hip_processed_with_binary.dat"
REPORT = r"C:\Users\13308\CLionProjects\stellarium\util\gaiacat\lv03_field_diff_report.md"
BV_TSV = r"C:\Users\13308\CLionProjects\stellarium\util\gaiacat\lv03_bv_diff.tsv"


def load_stars_config():
    """Full starsConfig.json (catalogs + hipSpectralFile) or {} if not found."""
    candidates = [os.path.expandvars(r"%APPDATA%\Stellarium\stars\hip_gaia3\starsConfig.json"),
                  os.path.expanduser("~/.stellarium/stars/hip_gaia3/starsConfig.json")]
    for p in candidates:
        if os.path.exists(p):
            with open(p, encoding="utf-8") as f:
                return json.load(f)
    return {}


_CFG = load_stars_config()
_OFFICIAL_NAMES = {int(c["id"][5:]): c["fileName"] for c in _CFG.get("catalogs", [])}
_OFFICIAL_SP = _CFG.get("hipSpectralFile", "stars_hip_sp_0v0_6.cat")


def next_cat_name(name):
    """'stars_0_0v0_21.cat' -> 'stars_0_0v0_22.cat',
    'stars_hip_sp_0v0_6.cat' -> 'stars_hip_sp_0v0_7.cat' (minor + 1)."""
    for pat in (r"^(stars_\d+_\d+v\d+_)(\d+)(\.cat)$",
                r"^(stars_hip_sp_\d+v\d+_)(\d+)(\.cat)$"):
        m = re.match(pat, name)
        if m:
            return f"{m.group(1)}{int(m.group(2)) + 1}{m.group(3)}"
    return name


LEVELS = []
for _lv, _lo, _hi in [(0, -2.0, 6.0), (1, 6.0, 7.5), (2, 7.5, 9.0), (3, 9.0, 10.5)]:
    _oname = _OFFICIAL_NAMES.get(_lv, f"stars_{_lv}_0v0_1.cat")
    LEVELS.append((_lv, _oname, next_cat_name(_oname), _lo, _hi))

# Star1 tolerances (same as cmpcat)
TOL_POS_MAS = 2.0
TOL_VMAG = 0.005
TOL_PM = 0.02      # mas/yr
TOL_PLX = 0.05      # mas
TOL_RV = 0.5       # km/s


def load_table(path):
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        return [line.rstrip("\n").rstrip("\r") for line in f]


SP_TABLE = load_table(os.path.join(OFFICIAL, _OFFICIAL_SP))
OT_TABLE = load_table(os.path.join(OFFICIAL, "object_types_v0_1.cat"))

our_sp_path = os.path.join(OURS, next_cat_name(_OFFICIAL_SP))
OUR_SP_TABLE = load_table(our_sp_path) if os.path.exists(our_sp_path) else SP_TABLE
our_ot_path = os.path.join(OURS, "object_types_v0_1.cat")
OUR_OT_TABLE = load_table(our_ot_path) if os.path.exists(our_ot_path) else OT_TABLE


def sp_name(i, side="A"):
    """Decode spectral type index using the appropriate table (A=official, B=ours)."""
    t = SP_TABLE if side == "A" else OUR_SP_TABLE
    return t[i] if 0 <= i < len(t) else f"?{i}"


def ot_name(i, side="A"):
    """Decode object type index using the appropriate table (A=official, B=ours)."""
    t = OT_TABLE if side == "A" else OUR_OT_TABLE
    return t[i] if 0 <= i < len(t) else f"NEW({i})"


R2D = 180.0 / math.pi


def decode_star1(buf, zone):  # 17-field unpack + derived values
    # pylint: disable=too-many-locals
    (gaia_id, x0, x1, x2, dx0, dx1, dx2, bv, vmag, plx, plxe, rv, sp, otype,
     h0, h1, h2) = struct.unpack("<q6i2h2HhHB3B", buf)
    nx, ny, nz = x0 / 2e9, x1 / 2e9, x2 / 2e9
    ra = math.atan2(ny, nx) * R2D
    if ra < 0:
        ra += 360.0
    r = math.sqrt(nx * nx + ny * ny + nz * nz)
    dec = math.asin(nz / r) * R2D if r > 0 else 0.0
    rad, dcd = ra / R2D, dec / R2D
    p0, p1 = -math.sin(rad), math.cos(rad)
    q0, q1, q2 = (-math.sin(dcd) * math.cos(rad), -math.sin(dcd) * math.sin(rad),
                  math.cos(dcd))
    pmra = (dx0 * p0 + dx1 * p1) / 1000.0
    pmdec = (dx0 * q0 + dx1 * q1 + dx2 * q2) / 1000.0
    comb = h0 | (h1 << 8) | (h2 << 16)
    hip, comp = comb >> 5, comb & 31
    key = gaia_id if gaia_id > 0 else ((1 << 62) | comb)
    return dict(key=key, gaia_id=gaia_id, ra=ra, dec=dec, pmra=pmra, pmdec=pmdec,
                vmag=vmag / 1000.0, bv=bv / 1000.0, plx=plx / 50.0, plxe=plxe / 100.0,
                rv=rv / 10.0, sp=sp, otype=otype, hip=hip, comp=comp, zone=zone,
                raw=(x0, x1, x2, dx0, dx1, dx2, bv, vmag, plx, plxe, rv, sp, otype, comb))


def read_cat(path):
    with open(path, "rb") as f:
        hdr = struct.unpack("<6I", f.read(24))
        level = hdr[4]
        f.read(4)
        nz = 20 * (4 ** level) + 1
        counts = struct.unpack(f"<{nz}I", f.read(4 * nz))
        stars = {}
        dups = []
        for z in range(nz):
            for _ in range(counts[z]):
                buf = f.read(48)
                s = decode_star1(buf, z)
                if s["key"] in stars:
                    dups.append(s["key"])
                stars[s["key"]] = s
        return stars, dups


def read_star2_keys(path):
    """gaia_id -> (vmag, zone) for a Star2 catalog (lv4-6 cross-reference)."""
    out = {}
    with open(path, "rb") as f:
        hdr = struct.unpack("<6I", f.read(24))
        level = hdr[4]
        f.read(4)
        nz = 20 * (4 ** level) + 1
        counts = struct.unpack(f"<{nz}I", f.read(4 * nz))
        for z in range(nz):
            for _ in range(counts[z]):
                buf = f.read(32)
                gaia_id, vmag = struct.unpack("<q", buf[:8])[0], struct.unpack("<h", buf[26:28])[0]
                out[gaia_id] = (vmag / 1000.0, z)
    return out


def read_simbad(path):
    """hip → dict(V, B, rv, sp, sid, comp, prov) for cause attribution."""
    out = {}
    if not os.path.exists(path):
        return out
    df = pd.read_csv(path, low_memory=False)
    for _, r in df.iterrows():
        if pd.isna(r["hip"]):
            continue
        hip = int(r["hip"])
        comp = 0
        if "componentid" in df.columns and not pd.isna(r["componentid"]):
            comp = int(float(r["componentid"]))
        sid = int(float(r["source_id"])) if not pd.isna(r["source_id"]) else 0
        sid_str = str(sid) if sid > 0 else ""

        def flt(col):
            if col not in df.columns or pd.isna(r[col]):
                return float("nan")
            try:
                return float(r[col])
            except (ValueError, TypeError):
                return float("nan")
        prov = ""
        if "source_id_provenance" in df.columns and not pd.isna(r["source_id_provenance"]):
            prov = str(r["source_id_provenance"])
        out[(hip, comp)] = dict(sid=sid_str,
                                V=flt("V"), B=flt("B"), rv=flt("rvz_radvel"),
                                sp=str(r["sp_type"]) if "sp_type" in df.columns and not pd.isna(r["sp_type"]) else "",
                                prov=prov)
    return out


def ang_diff_mas(a, b):
    dra = a["ra"] - b["ra"]
    if dra > 180:
        dra -= 360
    if dra < -180:
        dra += 360
    cosd = math.cos((a["dec"] + b["dec"]) / 2 / R2D)
    return dra * 3600000.0 * cosd, (a["dec"] - b["dec"]) * 3600000.0


def fmt_key(s):
    if s["gaia_id"] > 0:
        base = f"Gaia {s['gaia_id']}"
    else:
        base = "(no Gaia id)"
    if s["hip"] > 0:
        comp_label = " ABCD"[s["comp"]] if 0 < s["comp"] <= 4 else (f"c{s['comp']}" if s["comp"] else "")
        base += f" / HIP {s['hip']}{comp_label}"
    return base


# ---------------------------------------------------------------- load
print("loading catalogs...")
off, our = {}, {}
dup_notes = []
for lv, fn, fn_ours, _, _ in LEVELS:
    off[lv], d1 = read_cat(os.path.join(OFFICIAL, fn))
    our[lv], d2 = read_cat(os.path.join(OURS, fn_ours))
    for k in d1:
        dup_notes.append(f"Official lv{lv} duplicate match key: {k}")
    for k in d2:
        dup_notes.append(f"Our lv{lv} duplicate match key: {k}")
simbad = read_simbad(SIMBAD)
print("loading official lv4 for cross-reference...")
off_lv4 = read_star2_keys(os.path.join(OFFICIAL, "stars_4_1v0_6.cat"))
print("loading done")

# global index: key → (side, lv, star)
glob_a = {}
for lv, _, _, _, _ in LEVELS:
    for k, s in off[lv].items():
        glob_a[k] = ("A", lv, s)
our_glob = {}
for lv, _, _, _, _ in LEVELS:
    for k, s in our[lv].items():
        our_glob[k] = (lv, s)

lines = []


def w(s=""):
    lines.append(s)


w("# lv0-3 Star-by-Star Field Comparison: Official Catalog vs gaiahip2cat Output")
w()
w("- **A** = official catalog `stars/hip_gaia3/`")
w("- **B** = gaiahip2cat output `F:/gaiahip2cat_out/`")
w("- **Match key**: `gaia_id`; for gaia_id==0 uses `HIP<<5|component` (same as cmpcat)")
w("- **Global matching** (across zones): zone differences are classified separately, not counted as only-in (unlike cmpcat's per-zone matching)")
w("- **Tolerances** (Star1, same as cmpcat): position ±2 mas/axis, V ±5 mmag, pm ±0.02 mas/yr, plx ±0.05 mas, rv ±0.5 km/s")
w("- **B–V differences** (known method difference: BP‑RP polynomial vs official synthetic photometry) are excluded from this report; full listing in `lv03_bv_diff.tsv`")
w()
w("All tables below are sorted by match key (gaia_id, or HIP composite for no-Gaia-id stars).  "
  "Stars within each category table appear in ascending key order.")
w()
w("## Category legend")
w()
w("| Label | Meaning | Primary cause |")
w("|-------|---------|---------------|")
w("| ZONE  | Same star, different zone index | ERFA past/future global-zone boundary (era difference on the ±210 kyr analysis) |")
w("| POS   | Position exceeds tolerance | SIMBAD→Gaia cross‑match resolved to a different source in a different era, or binary component handling difference |")
w("| PM    | Proper motion exceeds tolerance | same as POS / binary-resolution era difference |")
w("| PLX   | Parallax or its error exceeds tolerance | same as POS |")
w("| V     | V magnitude exceeds tolerance | SIMBAD photometry revised between eras |")
w("| RV    | Radial velocity exceeds tolerance | SIMBAD `rvz_radvel` revised between eras (largest class) |")
w("| SP    | Spectral-type table index differs | SIMBAD `sp_type` revised between eras |")
w("| OTYPE | Object-type table index differs | SIMBAD `otype` re‑classified between eras (e.g. `*` ↔ `V*` ↔ `RG*`) |")
w("| HIP   | HIP number / component differs | Binary component assignment difference between eras |")
w()

if dup_notes:
    w("## Duplicate match-key warnings")
    w()
    for n in dup_notes:
        w(f"- {n}")
    w()

bv_rows = ["level\tkey\tgaia_id\thip\tcomp\tzoneA\tzoneB\tdBV_mag\tdV_mag\tvmagA\tvmagB"]


def prov_note(hip, comp):
    """Look up the source-id provenance for a (hip,comp) pair in the SIMBAD dat."""
    si = simbad.get((hip, comp))
    if not si and comp > 0:
        si = simbad.get((hip, 0))   # manually patched component: original dat row is comp=0
    if si and si.get("prov"):
        return " " + si["prov"]
    return ""


for lv, fn, fn_ours, lo, hi in LEVELS:
    A, B = off[lv], our[lv]
    w(f"## Level {lv}  (official {fn}, ours {fn_ours}, V ∈ ({lo}, {hi}],  A {len(A)} stars / B {len(B)} stars)")
    w()
    keys_a, keys_b = set(A), set(B)
    both = keys_a & keys_b
    only_a = keys_a - keys_b
    only_b = keys_b - keys_a

    # ---- matched: classify field differences
    cats = defaultdict(list)       # category → rows
    n_ident_raw = 0
    n_within_tol = 0
    n_bv = 0
    within_tol_rows = []
    for k in sorted(both):
        a, b = A[k], B[k]
        if a["raw"] == b["raw"] and a["zone"] == b["zone"]:
            n_ident_raw += 1
            continue
        dra, dde = ang_diff_mas(a, b)
        dpmra, dpmde = a["pmra"] - b["pmra"], a["pmdec"] - b["pmdec"]
        dv, dbv = a["vmag"] - b["vmag"], a["bv"] - b["bv"]
        dplx, dplxe = a["plx"] - b["plx"], a["plxe"] - b["plxe"]
        drv = a["rv"] - b["rv"]

        bad_pos = abs(dra) > TOL_POS_MAS or abs(dde) > TOL_POS_MAS
        bad_pm = abs(dpmra) > TOL_PM or abs(dpmde) > TOL_PM
        bad_v = abs(dv) > TOL_VMAG
        bad_plx = abs(dplx) > TOL_PLX
        bad_rv = abs(drv) > TOL_RV
        bad_bv = abs(dbv) > 0.05
        bad_zone = a["zone"] != b["zone"]
        bad_plxe = abs(dplxe) > 0.05
        bad_sp = a["sp"] != b["sp"]
        bad_ot = a["otype"] != b["otype"]
        bad_hip = (a["hip"], a["comp"]) != (b["hip"], b["comp"])

        if not any([bad_pos, bad_pm, bad_v, bad_plx, bad_rv, bad_bv, bad_zone,
                    bad_plxe, bad_sp, bad_ot, bad_hip]):
            # raw differs but every physical field within tolerance
            if a["raw"][:6] == b["raw"][:6] and a["raw"][7:] == b["raw"][7:]:
                # only the B‑V millimag differs: known method difference → TSV
                bv_rows.append(f"{lv}\t{k}\t{a['gaia_id']}\t{a['hip']}\t{a['comp']}\t"
                   f"{a['zone']}\t{b['zone']}\t{dbv:+.3f}\t{dv:+.3f}\t"
                   f"{a['vmag']:.3f}\t{b['vmag']:.3f}")
                n_bv += 1
                continue
            n_within_tol += 1
            pn = prov_note(b["hip"], b["comp"]) if b["hip"] > 0 else "gaia-only"
            within_tol_rows.append((b["hip"], b["comp"],
                        f"| {fmt_key(a)} | z{a['zone']} | dRA {dra:+.3f} dDE {dde:+.3f} mas | "
                        f"dpm {dpmra:+.3f}/{dpmde:+.3f} | dV {dv * 1000:+.0f} mmag  dBV {dbv * 1000:+.0f} mmag | "
                        f"dplx {dplx * 1000:+.0f} μas  dplxe {dplxe * 1000:+.0f} μas | drv {drv:+.2f} | {pn} |"))
            continue

        labels = []
        if bad_zone:
            labels.append("ZONE")
        if bad_pos:
            labels.append("POS")
        if bad_pm:
            labels.append("PM")
        if bad_plx or bad_plxe:
            labels.append("PLX")
        if bad_v:
            labels.append("V")
        if bad_bv:
            labels.append("BV")
        if bad_rv:
            labels.append("RV")
        if bad_sp:
            labels.append("SP")
        if bad_ot:
            labels.append("OTYPE")
        if bad_hip:
            labels.append("HIP")

        row = (k, a, b, labels, dict(dra=dra, dde=dde, dpmra=dpmra, dpmde=dpmde,
                                     dv=dv, dbv=dbv, dplx=dplx, dplxe=dplxe, drv=drv))
        # Primary category = first significant label; BV-only rows go to TSV
        pri = labels[0]
        if pri == "BV":
            bv_rows.append(f"{lv}\t{k}\t{a['gaia_id']}\t{a['hip']}\t{a['comp']}\t"
                           f"{a['zone']}\t{b['zone']}\t{dbv:+.3f}\t{dv:+.3f}\t"
                           f"{a['vmag']:.3f}\t{b['vmag']:.3f}")
            n_bv += 1
            continue
        cats[pri].append(row)

    w(f"- Byte-identical (raw + zone): **{n_ident_raw}**")
    w(f"- B‑V method difference (known; see `lv03_bv_diff.tsv`): **{n_bv}**")
    w(f"- Other fields differ but all within tolerance (quantisation): **{n_within_tol}**")
    w(f"- Beyond-tolerance / semantic differences: **{sum(len(v) for v in cats.values())}**")
    w(f"- Only in A (global match): **{len(only_a)}**,  only in B: **{len(only_b)}**")
    w()

    # ---- only-in analysis
    w(f"### Level {lv} — only‑in analysis")
    w()
    w("**Side**:  A‑only = official has it, we do not;  B‑only = we have it, official does not.")
    w("The position / V / zone column reflects the record from that side.")
    w("The Provenance column shows how the Gaia source_id was obtained in our SIMBAD input"
      " (`source_id_provenance` column of `hip_processed_with_binary.dat`).")
    w()

    pairs_used_b = set()
    paired_to_a = {}   # B‑key → (A star info dict, distance)
    rows_oa = []
    for k in sorted(only_a):
        a = A[k]
        if k in our_glob:
            blv, bs = our_glob[k]
            dv = a["vmag"] - bs["vmag"]
            if a["hip"] > 0 and bs["hip"] == 0:
                verdict = (f"HIP component removed (official hip={a['hip']}/{a['comp']}, "
                           f"we have Gaia‑only → lv{blv})")
            elif a["hip"] == 0 and bs["hip"] > 0:
                verdict = (f"HIP component added (our hip={bs['hip']}/{bs['comp']}, "
                           f"lv{blv} all‑HIP rule; official Gaia‑only at this level)")
                if abs(dv) > TOL_VMAG:
                    verdict += f"; also V diff {dv:+.3f}"
            else:
                verdict = f"Level shift (V diff {dv:+.3f})"
            rows_oa.append(("A‑only", a,
                 f"In our lv{blv} (z{bs['zone']}, V={bs['vmag']:.3f}, hip={bs['hip']})",
                             verdict + prov_note(bs["hip"], bs["comp"])))
            continue
        # positional pair among only_b?
        best, bestd = None, 1e9
        for k2 in only_b:
            if k2 in pairs_used_b:
                continue
            bs = B[k2]
            dra_, dde_ = ang_diff_mas(a, bs)
            d = math.hypot(dra_, dde_)
            if d < bestd:
                best, bestd = (k2, bs), d
        if best and bestd < 3600000.0:  # within 1°
            k2, bs = best
            pairs_used_b.add(k2)
            paired_to_a[k2] = (a, bestd)
            why = []
            if a["gaia_id"] != bs["gaia_id"]:
                why.append(f"gaia_id differs A={a['gaia_id']}  B={bs['gaia_id']}")
            if (a["hip"], a["comp"]) != (bs["hip"], bs["comp"]):
                why.append(f"HIP/comp differs A={a['hip']}/{a['comp']}  B={bs['hip']}/{bs['comp']}")
            if bestd > TOL_POS_MAS:
                why.append(f"position offset {bestd / 1000:.3f} arcsec")
            rows_oa.append(("A‑only", a,
                 f"Paired with B key ({fmt_key(bs)}, z{bs['zone']}, V={bs['vmag']:.3f})",
                             "Match key differs: " + "; ".join(why) + prov_note(bs["hip"], bs["comp"])))
            continue
        # nowhere in our lv0‑3 — check SIMB input
        si = simbad.get((a["hip"], a["comp"]))
        note = "Not in our lv0‑3 output (bin / lv4+ not searched)"
        if si:
            note += f"; our SIMBAD input V={si['V']}"
            if si.get("prov"):
                note += f" [prov: {si['prov']}]"
            if si["V"] == si["V"] and si["V"] > hi:
                note += f" (>{hi}, would fall to a fainter level)"
        else:
            note += "; this HIP/comp not in our SIMBAD input"
        rows_oa.append(("A‑only", a, "Unpaired", note))

    for k in sorted(only_b):
        b = B[k]
        if k in paired_to_a:
            a_p, dist = paired_to_a[k]
            rows_oa.append(("B‑only", b,
                 f"Paired with A key ({fmt_key(a_p)}, z{a_p['zone']}, V={a_p['vmag']:.3f}, dist {dist / 1000:.3f} ″)",
                 "Match key differs (positional pair, see the corresponding A‑only row)" + prov_note(b["hip"], b["comp"])))
            continue
        if k in pairs_used_b:
            continue
        if k in glob_a:
            aside, alv, astar = glob_a[k]
            dv = b["vmag"] - astar["vmag"]
            if b["hip"] > 0 and astar["hip"] == 0:
                verdict = (f"HIP component added (our hip={b['hip']}/{b['comp']}, "
                           f"lv2 all‑HIP rule; official Gaia‑only → lv{alv})")
                if abs(dv) > TOL_VMAG:
                    verdict += f"; also V diff {dv:+.3f}"
            else:
                verdict = f"Level shift (V diff {dv:+.3f})"
            rows_oa.append(("B‑only", b,
                 f"In official lv{alv} (z{astar['zone']}, V={astar['vmag']:.3f}, hip={astar['hip']})",
                             verdict + prov_note(b["hip"], b["comp"])))
        elif b["gaia_id"] > 0 and b["gaia_id"] in off_lv4:
            v4, z4 = off_lv4[b["gaia_id"]]
            rows_oa.append(("B‑only", b,
                 f"In official lv4 (z{z4}, V={v4:.3f})",
                 "Level shift (official V > 10.5 → lv4)" + prov_note(b["hip"], b["comp"])))
        else:
            si = simbad.get((b["hip"], b["comp"]))
            note = "Official lv0‑4 all empty (lv0‑3 + lv4 checked)"
            if si and si.get("prov"):
                note += f" [prov: {si['prov']}]"
            if b["gaia_id"] == 0 and si:
                note += f"; SIMBAD sid={si['sid']}"
            rows_oa.append(("B‑only", b, "Unpaired", note))

    if rows_oa:
        w("| Side | Star | Position / V / zone | Other‑side info | Verdict |")
        w("|------|------|----------------------|-----------------|---------|")
        for side, a, bside, verdict in rows_oa:
            w(f"| {side} | {fmt_key(a)} | RA {a['ra']:.5f}  Dec {a['dec']:+.5f}  V={a['vmag']:.3f}  z{a['zone']} | {bside} | {verdict} |")
        w()
    else:
        w("(none)")
        w()

    # ---- category tables for this level
    w(f"### Level {lv} — matched stars with semantic differences (by category)")
    w("Stars are sorted by match key (gaia_id).  Each star appears only once, under the"
      " highest-priority category it triggers (priority order: ZONE > POS > PM > PLX > V > BV > RV > SP > OTYPE > HIP).")
    w()
    if not cats:
        w("(none)")
        w()
    for pri in ["ZONE", "POS", "PM", "PLX", "V", "BV", "RV", "SP", "OTYPE", "HIP"]:
        rows = cats.get(pri)
        if not rows:
            continue
        w(f"#### Primary category **{pri}** ({len(rows)} stars)")
        w()
        w("| Star | zone A→B | dRA/dDE (mas) | dpm (mas/yr) | dV | dBV | dplx/dplxe (mas) | drv (km/s) | Other fields | Labels | Provenance |")
        w("|------|----------|-------------|----------|----|-----|---------------|--------|--------------|--------|------------|")
        for k, a, b, labels, d in rows:
            other = []
            if a["sp"] != b["sp"]:
                other.append(f"sp {sp_name(a['sp'], 'A')}→{sp_name(b['sp'], 'B')}")
            if a["otype"] != b["otype"]:
                other.append(f"ot {ot_name(a['otype'], 'A')}→{ot_name(b['otype'], 'B')}")
            if (a["hip"], a["comp"]) != (b["hip"], b["comp"]):
                other.append(f"hip {a['hip']}/{a['comp']}→{b['hip']}/{b['comp']}")
            pn = prov_note(b["hip"], b["comp"]) if b["hip"] > 0 else "gaia-only"
            w(f"| {fmt_key(a)} | {a['zone']}→{b['zone']} | {d['dra']:+.3f}/{d['dde']:+.3f} | "
              f"{d['dpmra']:+.3f}/{d['dpmde']:+.3f} | {d['dv']:+.3f} | {d['dbv']:+.3f} | "
              f"{d['dplx']:+.3f}/{d['dplxe']:+.3f} | {d['drv']:+.2f} | "
              f"{' '.join(other)} | {','.join(labels)} | {pn} |")
        w()

    if within_tol_rows:
        w(f"<details><summary>Level {lv} — within-tolerance raw differences ({len(within_tol_rows)} stars, quantisation)</summary>")
        w()
        w("| Star | zone | Position | pm | V/BV | plx | rv | Provenance |")
        w("|------|------|----------|----|------|-----|----|------------|")
        for hip, comp, r in within_tol_rows:
            w(r)
        w()
        w("</details>")
        w()


with open(BV_TSV, "w", encoding="utf-8") as f:
    f.write("\n".join(bv_rows) + "\n")
print("wrote", BV_TSV, len(bv_rows) - 1, "rows")
with open(REPORT, "w", encoding="utf-8") as f:
    f.write("\n".join(lines) + "\n")
print("wrote", REPORT, len(lines), "lines")
