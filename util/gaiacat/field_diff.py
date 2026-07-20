#!/usr/bin/env python3
# field_diff.py — exhaustive star-by-star field comparison of two Star1 catalog
# sets (official vs hip2cat output), levels 0-3. Matches globally by gaia_id
# (HIP<<5|comp when gaia_id==0), classifies every differing star by cause.
# Output: Markdown report with per-star tables (no differing star omitted).

import struct, sys, math, os
from collections import defaultdict

OFFICIAL = r"C:\Users\13308\CLionProjects\stellarium\stars\hip_gaia3"
OURS     = r"F:\hip2cat_out"
SIMBAD   = r"C:\Users\13308\CLionProjects\stellarium_star_catalogs\simbad_query_results\hip_processed_with_binary.dat"
REPORT   = r"C:\Users\13308\CLionProjects\stellarium\util\gaiacat\lv03_field_diff_report.md"
BV_TSV   = r"C:\Users\13308\CLionProjects\stellarium\util\gaiacat\lv03_bv_diff.tsv"

LEVELS = [
    (0, "stars_0_0v0_21.cat", -2.0, 6.0),
    (1, "stars_1_0v0_16.cat", 6.0, 7.5),
    (2, "stars_2_0v0_17.cat", 7.5, 9.0),
    (3, "stars_3_0v0_10.cat", 9.0, 10.5),
]

# Star1 tolerances (same as cmpcat)
TOL_POS_MAS = 2.0
TOL_VMAG    = 0.005
TOL_PM      = 0.02      # mas/yr
TOL_PLX     = 0.05      # mas
TOL_RV      = 0.5       # km/s

def load_table(path):
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        return [l.rstrip("\n").rstrip("\r") for l in f]

SP_TABLE = load_table(os.path.join(OFFICIAL, "stars_hip_sp_0v0_6.cat"))
OT_TABLE = load_table(os.path.join(OFFICIAL, "object_types_v0_1.cat"))

def sp_name(i):
    return SP_TABLE[i] if 0 <= i < len(SP_TABLE) else f"?{i}"

def ot_name(i):
    return OT_TABLE[i] if 0 <= i < len(OT_TABLE) else f"NEW({i})"

R2D = 180.0 / math.pi

def decode_star1(buf, zone):
    (gaia_id, x0, x1, x2, dx0, dx1, dx2, bv, vmag, plx, plxe, rv, sp, otype,
     h0, h1, h2) = struct.unpack("<q6i2h2HhHB3B", buf)
    nx, ny, nz = x0 / 2e9, x1 / 2e9, x2 / 2e9
    ra = math.atan2(ny, nx) * R2D
    if ra < 0: ra += 360.0
    r = math.sqrt(nx*nx + ny*ny + nz*nz)
    dec = math.asin(nz / r) * R2D if r > 0 else 0.0
    rad, dcd = ra / R2D, dec / R2D
    p0, p1 = -math.sin(rad), math.cos(rad)
    q0, q1, q2 = -math.sin(dcd)*math.cos(rad), -math.sin(dcd)*math.sin(rad), math.cos(dcd)
    pmra  = (dx0*p0 + dx1*p1) / 1000.0
    pmdec = (dx0*q0 + dx1*q1 + dx2*q2) / 1000.0
    comb = h0 | (h1 << 8) | (h2 << 16)
    hip, comp = comb >> 5, comb & 31
    key = gaia_id if gaia_id > 0 else ((1 << 62) | comb)
    return dict(key=key, gaia_id=gaia_id, ra=ra, dec=dec, pmra=pmra, pmdec=pmdec,
                vmag=vmag/1000.0, bv=bv/1000.0, plx=plx/50.0, plxe=plxe/100.0,
                rv=rv/10.0, sp=sp, otype=otype, hip=hip, comp=comp, zone=zone,
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
    """hip -> dict(V, B, rv, sp, sid, comp) for cause attribution."""
    out = {}
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        hdr = f.readline().rstrip("\n").split(",")
        idx = {h: i for i, h in enumerate(hdr)}
        def col(name): return idx.get(name, -1)
        c_hip, c_comp, c_sid = col("hip"), col("componentid"), col("source_id")
        c_V, c_B, c_rv, c_sp = col("V"), col("B"), col("rvz_radvel"), col("sp_type")
        for line in f:
            line = line.rstrip("\n")
            if not line: continue
            fld = line.split(",")
            try: hip = int(fld[c_hip])
            except: continue
            def flt(i):
                if i < 0 or i >= len(fld) or fld[i] == "": return float("nan")
                try: return float(fld[i])
                except: return float("nan")
            comp = 0
            if c_comp >= 0 and c_comp < len(fld) and fld[c_comp]:
                try: comp = int(float(fld[c_comp]))
                except: comp = 0
            out[(hip, comp)] = dict(sid=fld[c_sid] if c_sid >= 0 else "",
                                    V=flt(c_V), B=flt(c_B), rv=flt(c_rv),
                                    sp=fld[c_sp] if c_sp >= 0 else "")
    return out

def ang_diff_mas(a, b):
    dra = (a["ra"] - b["ra"])
    if dra > 180: dra -= 360
    if dra < -180: dra += 360
    cosd = math.cos((a["dec"] + b["dec"]) / 2 / R2D)
    return dra * 3600000.0 * cosd, (a["dec"] - b["dec"]) * 3600000.0

def fmt_key(s):
    if s["gaia_id"] > 0:
        base = f"Gaia {s['gaia_id']}"
    else:
        base = "(no Gaia id)"
    if s["hip"] > 0:
        comp = " ABCD"[s["comp"]] if 0 < s["comp"] <= 4 else (f"c{s['comp']}" if s["comp"] else "")
        base += f" / HIP {s['hip']}{comp}"
    return base

# ---------------------------------------------------------------- load
print("loading catalogs...")
off, our = {}, {}
dup_notes = []
for lv, fn, _, _ in LEVELS:
    off[lv], d1 = read_cat(os.path.join(OFFICIAL, fn))
    our[lv], d2 = read_cat(os.path.join(OURS, fn))
    for k in d1: dup_notes.append(f"官方 lv{lv} 匹配键重复: {k}")
    for k in d2: dup_notes.append(f"我方 lv{lv} 匹配键重复: {k}")
simbad = read_simbad(SIMBAD)
print("loading official lv4 for cross-reference...")
off_lv4 = read_star2_keys(os.path.join(OFFICIAL, "stars_4_1v0_6.cat"))
print("loading done")

# global index: key -> (side, lv, star)
glob = {}
for lv, _, _, _ in LEVELS:
    for k, s in off[lv].items(): glob[k] = ("A", lv, s)
our_glob = {}
for lv, _, _, _ in LEVELS:
    for k, s in our[lv].items(): our_glob[k] = (lv, s)

lines = []
def w(s=""): lines.append(s)

w("# lv0-3 逐星字段对比报告:官方目录 vs hip2cat 输出")
w()
w("- A = 官方目录 `stars/hip_gaia3/`")
w("- B = hip2cat 输出 `F:/hip2cat_out/`")
w("- 匹配键:gaia_id;gaia_id==0 时用 HIP<<5|component(与 cmpcat 一致)")
w("- 全局匹配(跨 zone),zone 不同单独归类,不会像 cmpcat 那样计为 only-in")
w("- 容差(Star1,同 cmpcat):pos ±2 mas/轴,V ±5 mmag,pm ±0.02 mas/yr,plx ±0.05 mas,rv ±0.5 km/s")
w("- B-V 差异(已知方法差异:BP-RP 多项式 vs 官方合成测光)不在本文列出,全量见 lv03_bv_diff.tsv")
w()
w("## 类别成因图例")
w()
w("| 标签 | 含义 | 主要成因 |")
w("|---|---|---|")
w("| ZONE | 同一颗星 zone 编号不同 | ERFA 过去/未来 global-zone 判定边界(官方 ±210 kyr 分析的时代差异) |")
w("| POS/PM/PLX | 位置/自行/视差超容差 | SIMBAD→Gaia cross-match 在不同时代解析到不同源,或双星分量处理差异 |")
w("| V | V 星等超容差 | SIMBAD 测光随时代修订 |")
w("| RV | 径向速度超容差 | SIMBAD rvz_radvel 随时代修订(最大类) |")
w("| SP | 光谱型索引不同 | SIMBAD sp_type 随时代修订 |")
w("| OTYPE | 天体类型索引不同 | SIMBAD otype 随时代修订(如 * ↔ V* ↔ RG*) |")
w("| HIP | HIP 号/分量不同 | 双星分量归属处理差异 |")
w()
if dup_notes:
    w("## 匹配键重复警告")
    w()
    for n in dup_notes: w(f"- {n}")
    w()

bv_rows = ["level\tkey\tgaia_id\thip\tcomp\tzoneA\tzoneB\tdBV_mag\tdV_mag\tvmagA\tvmagB"]

grand_tables = defaultdict(list)   # category -> list of row strings
only_rows = []

for lv, fn, lo, hi in LEVELS:
    A, B = off[lv], our[lv]
    w(f"## level {lv}({fn},V ∈ ({lo}, {hi}],A {len(A)} 颗 / B {len(B)} 颗)")
    w()
    keys_a = set(A); keys_b = set(B)
    both = keys_a & keys_b
    only_a = keys_a - keys_b
    only_b = keys_b - keys_a

    # ---- matched: classify field differences
    cats = defaultdict(list)   # category -> rows
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
        dpmra = a["pmra"] - b["pmra"]
        dpmde = a["pmdec"] - b["pmdec"]
        dv  = a["vmag"] - b["vmag"]
        dbv = a["bv"] - b["bv"]
        dplx = a["plx"] - b["plx"]
        dplxe = a["plxe"] - b["plxe"]
        drv = a["rv"] - b["rv"]
        bad_pos  = abs(dra) > TOL_POS_MAS or abs(dde) > TOL_POS_MAS
        bad_pm   = abs(dpmra) > TOL_PM or abs(dpmde) > TOL_PM
        bad_v    = abs(dv) > TOL_VMAG
        bad_plx  = abs(dplx) > TOL_PLX
        bad_rv   = abs(drv) > TOL_RV
        bad_bv   = abs(dbv) > 0.05
        bad_zone = a["zone"] != b["zone"]
        bad_plxe = abs(dplxe) > 0.05
        bad_sp   = a["sp"] != b["sp"]
        bad_ot   = a["otype"] != b["otype"]
        bad_hip  = (a["hip"], a["comp"]) != (b["hip"], b["comp"])
        if not any([bad_pos, bad_pm, bad_v, bad_plx, bad_rv, bad_bv, bad_zone,
                    bad_plxe, bad_sp, bad_ot, bad_hip]):
            # raw differs but every physical field within tolerance
            if a["raw"][:6] == b["raw"][:6] and a["raw"][7:] == b["raw"][7:]:
                # only the B-V millimag differs: known method difference -> TSV
                bv_rows.append(f"{lv}\t{k}\t{a['gaia_id']}\t{a['hip']}\t{a['comp']}\t"
                               f"{a['zone']}\t{b['zone']}\t{dbv:+.3f}\t{dv:+.3f}\t"
                               f"{a['vmag']:.3f}\t{b['vmag']:.3f}")
                n_bv += 1
                continue
            n_within_tol += 1
            within_tol_rows.append(
                f"| {fmt_key(a)} | z{a['zone']} | dRA {dra:+.3f} dDE {dde:+.3f} mas | "
                f"dpm {dpmra:+.3f}/{dpmde:+.3f} | dV {dv*1000:+.0f}mmag dBV {dbv*1000:+.0f}mmag | "
                f"dplx {dplx*1000:+.0f}uas dplxe {dplxe*1000:+.0f}uas | drv {drv:+.2f} |")
            continue
        labels = []
        if bad_zone: labels.append("ZONE")
        if bad_pos:  labels.append("POS")
        if bad_pm:   labels.append("PM")
        if bad_plx or bad_plxe: labels.append("PLX")
        if bad_v:    labels.append("V")
        if bad_bv:   labels.append("BV")
        if bad_rv:   labels.append("RV")
        if bad_sp:   labels.append("SP")
        if bad_ot:   labels.append("OTYPE")
        if bad_hip:  labels.append("HIP")
        row = (k, a, b, labels, dict(dra=dra, dde=dde, dpmra=dpmra, dpmde=dpmde,
                                     dv=dv, dbv=dbv, dplx=dplx, dplxe=dplxe, drv=drv))
        # primary category for grouping (first significant); BV-only rows go to TSV
        pri = labels[0]
        if pri == "BV":
            bv_rows.append(f"{lv}\t{k}\t{a['gaia_id']}\t{a['hip']}\t{a['comp']}\t"
                           f"{a['zone']}\t{b['zone']}\t{dbv:+.3f}\t{dv:+.3f}\t"
                           f"{a['vmag']:.3f}\t{b['vmag']:.3f}")
            n_bv += 1
            continue
        cats[pri].append(row)
        grand_tables[pri].append((lv, row))

    w(f"- 完全一致(raw 逐字节 + zone 相同):**{n_ident_raw}**")
    w(f"- B-V 差异(已知方法差异,见 lv03_bv_diff.tsv):**{n_bv}**")
    w(f"- 其他字段有差异但全部在容差内(量化舍入):**{n_within_tol}**")
    w(f"- 超出容差/有语义差异:**{sum(len(v) for v in cats.values())}**")
    w(f"- only in A(全局匹配后):**{len(only_a)}**,only in B:**{len(only_b)}**")
    w()

    # ---- only-in analysis: cross-level + positional pairing
    w(f"### level {lv}:only-in 逐星分析")
    w()
    w("侧别:仅A = 官方有我无;仅B = 我方有官方无。位置/V/zone 列取该侧的记录值。")
    w()
    pairs_used_b = set()
    rows_oa = []
    for k in sorted(only_a):
        a = A[k]
        # same key in other level of B?
        if k in our_glob:
            blv, bs = our_glob[k]
            dv = a["vmag"] - bs["vmag"]
            if a["hip"] > 0 and bs["hip"] == 0:
                verdict = (f"HIP 分量移除时代差(官方 hip={a['hip']}/{a['comp']},"
                           f"我方为 Gaia-only 按星等归 lv{blv})")
            elif a["hip"] == 0 and bs["hip"] > 0:
                verdict = (f"HIP 分量新增时代差(我方 hip={bs['hip']}/{bs['comp']},"
                           f"lv{blv} all-HIP 规则;官方 Gaia-only 在本层)")
                if abs(dv) > TOL_VMAG:
                    verdict += f";另有 V 差异 {dv:+.3f}"
            else:
                verdict = f"跨层移动(V 差异 {dv:+.3f})"
            rows_oa.append(("仅A", a, f"在 B 的 lv{blv}(z{bs['zone']},V={bs['vmag']:.3f},hip={bs['hip']})",
                            verdict))
            continue
        # positional pair among only_b?
        best, bestd = None, 1e9
        for k2 in only_b:
            if k2 in pairs_used_b: continue
            bs = B[k2]
            dra, dde = ang_diff_mas(a, bs)
            d = math.hypot(dra, dde)
            if d < bestd: best, bestd = (k2, bs), d
        if best and bestd < 3600000.0:  # within 1 deg
            k2, bs = best
            pairs_used_b.add(k2)
            why = []
            if a["gaia_id"] != bs["gaia_id"]:
                why.append(f"gaia_id 不同 A={a['gaia_id']} B={bs['gaia_id']}")
            if (a["hip"], a["comp"]) != (bs["hip"], bs["comp"]):
                why.append(f"HIP/comp 不同 A={a['hip']}/{a['comp']} B={bs['hip']}/{bs['comp']}")
            if bestd > TOL_POS_MAS:
                why.append(f"位置差 {bestd/1000:.3f} arcsec")
            rows_oa.append(("仅A", a, f"配对 B 键 ({fmt_key(bs)},z{bs['zone']},V={bs['vmag']:.3f})",
                            "匹配键不同:" + ";".join(why)))
            continue
        # nowhere in B at any level: look at SIMBAD input
        si = simbad.get((a["hip"], a["comp"]))
        note = "B 的 lv0-3 产物中无(未查 bin/lv4+)"
        if si:
            note += f";我方 SIMBAD 输入 V={si['V']}"
            if si["V"] == si["V"] and si["V"] > hi:
                note += f"(>{hi},应落到更暗层)"
        else:
            note += ";我方 SIMBAD 输入中无此 HIP/comp"
        rows_oa.append(("仅A", a, "未配对", note))
    for k in sorted(only_b - pairs_used_b):
        b = B[k]
        if k in glob:
            aside, alv, astar = glob[k]
            dv = b["vmag"] - astar["vmag"]
            if b["hip"] > 0 and astar["hip"] == 0:
                verdict = (f"HIP 分量新增时代差(我方 hip={b['hip']}/{b['comp']},"
                           f"lv2 all-HIP 规则(V>7.5 必收);官方 Gaia-only 按星等归 lv{alv})")
                if abs(dv) > TOL_VMAG:
                    verdict += f";另有 V 差异 {dv:+.3f}"
            else:
                verdict = f"跨层移动(V 差异 {dv:+.3f})"
            rows_oa.append(("仅B", b, f"在 A 的 lv{alv}(z{astar['zone']},V={astar['vmag']:.3f},hip={astar['hip']})",
                            verdict))
        elif b["gaia_id"] > 0 and b["gaia_id"] in off_lv4:
            v4, z4 = off_lv4[b["gaia_id"]]
            rows_oa.append(("仅B", b, f"在 A 的 lv4(z{z4},V={v4:.3f})",
                            "跨层移动(官方 V>10.5 落入 lv4)"))
        else:
            si = simbad.get((b["hip"], b["comp"]))
            note = "A 的 lv0-4 均无(已查官方 lv0-3 + lv4)"
            if b["gaia_id"] == 0 and si:
                note += f";SIMBAD sid={si['sid']}"
            rows_oa.append(("仅B", b, "未配对", note))
    if rows_oa:
        w("| 侧别 | 星 | 位置/V/zone | 对侧情况 | 判定 |")
        w("|---|---|---|---|---|")
        for side, a, bside, verdict in rows_oa:
            w(f"| {side} | {fmt_key(a)} | RA {a['ra']:.5f} Dec {a['dec']:+.5f} V={a['vmag']:.3f} z{a['zone']} | {bside} | {verdict} |")
        w()
    else:
        w("无。")
        w()

    # ---- category tables for this level
    w(f"### level {lv}:匹配但字段有语义差异的星(分类)")
    w()
    if not cats:
        w("无。")
        w()
    for pri in ["ZONE", "POS", "PM", "PLX", "V", "BV", "RV", "SP", "OTYPE", "HIP"]:
        rows = cats.get(pri)
        if not rows: continue
        w(f"#### 主类别 {pri}({len(rows)} 颗)")
        w()
        w("| 星 | zone A→B | dRA/dDE (mas) | dpm (mas/yr) | dV | dBV | dplx/dplxe (mas) | drv (km/s) | 其他字段 | 标签 |")
        w("|---|---|---|---|---|---|---|---|---|---|")
        for k, a, b, labels, d in rows:
            other = []
            if a["sp"] != b["sp"]:
                other.append(f"sp {sp_name(a['sp'])}→{sp_name(b['sp'])}")
            if a["otype"] != b["otype"]:
                other.append(f"ot {ot_name(a['otype'])}→{ot_name(b['otype'])}")
            if (a["hip"], a["comp"]) != (b["hip"], b["comp"]):
                other.append(f"hip {a['hip']}/{a['comp']}→{b['hip']}/{b['comp']}")
            w(f"| {fmt_key(a)} | {a['zone']}→{b['zone']} | {d['dra']:+.3f}/{d['dde']:+.3f} | "
              f"{d['dpmra']:+.3f}/{d['dpmde']:+.3f} | {d['dv']:+.3f} | {d['dbv']:+.3f} | "
              f"{d['dplx']:+.3f}/{d['dplxe']:+.3f} | {d['drv']:+.2f} | "
              f"{' '.join(other)} | {','.join(labels)} |")
        w()

    if within_tol_rows:
        w(f"<details><summary>level {lv}:容差内的 raw 差异({len(within_tol_rows)} 颗,舍入/量化)</summary>")
        w()
        w("| 星 | zone | 位置 | pm | V/BV | plx | rv |")
        w("|---|---|---|---|---|---|---|")
        for r in within_tol_rows:
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
