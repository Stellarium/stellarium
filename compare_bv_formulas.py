#!/usr/bin/env python3
"""Compare B-V formulas against stars_8 catalog using SkyChart Gaia .dat data."""
import struct, os, math

CAT_PATH = r'C:\Users\13308\CLionProjects\stellarium\stars\hip_gaia3\stars_8_2v0_3.cat'
SKYCHART_DIR = r'F:\gaiadr3\vega.ap-i.net\gaia'
N_SAMPLE = 20000  # number of stars to sample

def g_to_v(g, c):
    return g + 0.02704 - 0.01424*c + 0.2156*c*c - 0.01426*c*c*c

# ── B-V formula 1: Gaia DR3 direct (G-V)-(G-B) ───
def bv_gaia(c):
    gmv = -0.02704 + 0.01424*c - 0.2156*c*c + 0.01426*c*c*c
    gmb =  0.01448 - 0.6874*c - 0.3604*c*c + 0.06718*c*c*c - 0.006061*c*c*c*c
    return max(0.0, min(gmv - gmb, 5.35))

# ── B-V formula 2: SkyChart 2-step ───────────────
def bv_skychart(c):
    bt_vt = -0.006482 + 0.7865*c - 0.3631*c*c + 0.93192*c*c*c - 0.4843*c*c*c*c + 0.06814*c*c*c*c*c
    if -0.25 < bt_vt < 0.5:
        bv = bt_vt - 0.006 - 0.1069*bt_vt + 0.1459*bt_vt*bt_vt
    elif 0.5 <= bt_vt < 2.0:
        bv = bt_vt - 0.007813*bt_vt - 0.1489*bt_vt*bt_vt + 0.03384*bt_vt*bt_vt*bt_vt
    else:
        bv = 0.850 * bt_vt
    return max(0.0, min(bv, 5.35))

# ── Geodesic Grid ────────────────────────────────
PHI=(1+math.sqrt(5))/2; NORM=math.sqrt(1+PHI*PHI); _a,_b=PHI/NORM,1/NORM
_IV=[(_a,-_b,0),(_a,_b,0),(-_a,_b,0),(-_a,-_b,0),(0,_a,-_b),(0,_a,_b),(0,-_a,_b),(0,-_a,-_b),(-_b,0,_a),(_b,0,_a),(_b,0,-_a),(-_b,0,-_a)]
_IF=[(1,0,10),(0,1,9),(0,9,6),(9,8,6),(0,7,10),(6,7,0),(7,6,3),(6,8,3),(11,10,7),(7,3,11),(3,2,11),(2,3,8),(10,11,4),(2,4,11),(5,4,2),(2,8,5),(4,1,10),(4,5,1),(5,9,1),(8,9,5)]
def _cr(a,b): return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
def _dt(a,b): return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]
def _no(v): ln=math.sqrt(v[0]**2+v[1]**2+v[2]**2); return (v[0]/ln,v[1]/ln,v[2]/ln) if ln>0 else v

class G:
    def __init__(s): s._c={}
    def _co(s,l,i):
        k=(l,i)
        if k in s._c: return s._c[k]
        if l<=0: c=_IF[i]; r=(_IV[c[0]],_IV[c[1]],_IV[c[2]])
        else:
            lv=l-1; ii=i>>2; c0,c1,c2=s._co(lv,ii)
            e0=_no((c1[0]+c2[0],c1[1]+c2[1],c1[2]+c2[2]))
            e1=_no((c2[0]+c0[0],c2[1]+c0[1],c2[2]+c0[2]))
            e2=_no((c0[0]+c1[0],c0[1]+c1[1],c0[2]+c1[2]))
            si=i&3
            if si==0: r=(c0,e2,e1)
            elif si==1: r=(e2,c1,e0)
            elif si==2: r=(e1,e0,c2)
            else: r=(e0,e1,e2)
        s._c[k]=r; return r
    def zn(s,x,y,z,lev):
        for i in range(20):
            c0,c1,c2=(_IV[_IF[i][j]] for j in range(3))
            if _dt(_cr(c0,c1),(x,y,z))>=-1e-10 and _dt(_cr(c1,c2),(x,y,z))>=-1e-10 and _dt(_cr(c2,c0),(x,y,z))>=-1e-10:
                idx=i
                for l in range(lev):
                    c0,c1,c2=s._co(l,idx)
                    e0=_no((c1[0]+c2[0],c1[1]+c2[1],c1[2]+c2[2]))
                    e1=_no((c2[0]+c0[0],c2[1]+c0[1],c2[2]+c0[2]))
                    e2=_no((c0[0]+c1[0],c0[1]+c1[1],c0[2]+c1[2]))
                    idx<<=2
                    if _dt(_cr(e1,e2),(x,y,z))<=1e-10: pass
                    elif _dt(_cr(e2,e0),(x,y,z))<=1e-10: idx+=1
                    elif _dt(_cr(e0,e1),(x,y,z))<=1e-10: idx+=2
                    else: idx+=3
                return idx
        return 0
grid=G()

# ── Sample SkyChart ──────────────────────────────
print(f'Sampling {N_SAMPLE} stars from SkyChart gaia3...')
candidates=[]; count=0
for sub in ['gaia3']:
    d=os.path.join(SKYCHART_DIR,sub)
    for zdir in sorted(os.listdir(d))[:15]:
        zp=os.path.join(d,zdir)
        for fn in sorted(os.listdir(zp))[:2]:
            with open(os.path.join(zp,fn),'rb') as fh: data=fh.read()
            for i in range(len(data)//38):
                off=i*38
                g=struct.unpack_from('<h',data,off+16)[0]/1000.0
                bp=struct.unpack_from('<h',data,off+18)[0]/1000.0
                rp=struct.unpack_from('<h',data,off+20)[0]/1000.0
                if abs(bp)>=300 or abs(rp)>=300: continue
                c=bp-rp; vp=g_to_v(g,c)
                if 16.75<=vp<18.0:
                    sid=struct.unpack_from('<Q',data,off+8)[0]
                    ra=struct.unpack_from('<I',data,off)[0]/3600000.0
                    dec=struct.unpack_from('<I',data,off+4)[0]/3600000.0-90.0
                    x,y,z=(math.cos(rm:=math.radians(ra))*math.cos(dm:=math.radians(dec)),math.sin(rm)*math.cos(dm),math.sin(dm))
                    zone=grid.zn(x,y,z,8)
                    candidates.append((sid,ra,dec,g,bp,rp,c,vp,bv_gaia(c),bv_skychart(c),zone))
                    count+=1
                    if count>=N_SAMPLE: break
            if count>=N_SAMPLE: break
        if count>=N_SAMPLE: break
    if count>=N_SAMPLE: break
print(f'Sampled {len(candidates)} candidates')

# ── Search candidates in stars_8 ─────────────────
print('Reading stars_8 zone table...')
with open(CAT_PATH,'rb') as f:
    h=f.read(28)
    level=struct.unpack_from('<I',h,16)[0]
    nz=20*(4**level)+1
    f.seek(28); zs=f.read(nz*4)
    sb=28+nz*4
zone_info=[]; off=sb
for z in range(nz):
    cnt=struct.unpack_from('<I',zs,z*4)[0]
    if cnt>0: zone_info.append((z,cnt,off))
    off+=cnt*16

by_zone={}
for c in candidates:
    by_zone.setdefault(c[-1],[]).append(c)

gaia_errs, sky_errs = [], []
bp_rp_vals, b_cats = [], []
matched=0

for z,cnt,off in zone_info:
    if z not in by_zone: continue
    targets=by_zone[z]; tids={t[0] for t in targets}
    found={}
    with open(CAT_PATH,'rb') as f:
        for i in range(cnt):
            f.seek(off+i*16); rec=f.read(16)
            sid=struct.unpack_from('<q',rec,0)[0]
            if sid in tids:
                rc=int.from_bytes(rec[8:11],'little')/36000.0
                dc=int.from_bytes(rec[11:14],'little')/36000.0-90.0
                bc=rec[14]*0.025-1.0
                vc=rec[15]*0.02+16.0
                found[sid]=(rc,dc,vc,bc)
    for sid,ra,dec,g,bp,rp,c,vp,bv_g,bv_s,zn in targets:
        if sid in found:
            _,_,_,bc = found[sid]
            gaia_errs.append(bv_g - bc)
            sky_errs.append(bv_s - bc)
            bp_rp_vals.append(c)
            b_cats.append(bc)
            matched+=1

# ── Print comparison ───────────────────────────── 
for name, errs in [('Gaia DR3 (G-V)-(G-B)', gaia_errs), ('SkyChart 2-step', sky_errs)]:
    errs.sort()
    m=len(errs)
    print(f'\n{"="*60}')
    print(f' {name}')
    print(f'{"="*60}')
    print(f' Samples: {m}')
    print(f' Median:  {errs[m//2]:+.4f}')
    print(f' Mean:    {sum(errs)/m:+.4f}')
    print(f' Std:     {math.sqrt(sum(x*x for x in errs)/m):.4f}')
    print(f' Min:     {errs[0]:+.4f}')
    print(f' Max:     {errs[-1]:+.4f}')
    print(f'\n Percentiles:')
    for p in [5,10,25,50,75,90,95]:
        idx=max(0,min(m-1,int(m*p/100))); print(f'  P{p:>2}: {errs[idx]:+.4f}')
    print(f'\n Error bins (0.025 mag = Star3 precision):')
    bins={}
    for d in errs:
        b=round(d/0.025)*0.025; bins[b]=bins.get(b,0)+1
    # Show central ±0.2 range only
    for b in sorted(bins.keys()):
        if -0.2 <= b <= 0.2:
            bar='#'*max(1,(bins[b]*60//max(bins.values())))
            print(f'  {b:+6.3f}: {bins[b]:>5} {bar}')
    # Also show outliers summary
    inside=sum(v for b,v in bins.items() if -0.025<=b<=0.025)
    within_2step=sum(v for b,v in bins.items() if -0.05<=b<=0.05)
    print(f'\n Within ±0.025 mag: {inside}/{m} = {100.0*inside/m:.1f}%')
    print(f' Within ±0.050 mag: {within_2step}/{m} = {100.0*within_2step/m:.1f}%')

    # BP-RP breakdown
    print(f'\n BP-RP range vs error:')
    for lo,hi in [(-0.5,0.5),(0.5,1.0),(1.0,1.5),(1.5,2.0),(2.0,5.0)]:
        sub=[errs[i] for i in range(m) if lo<=bp_rp_vals[i]<hi]
        if sub:
            sub.sort(); n=len(sub)
            print(f'  [{lo:>4.1f},{hi:>4.1f}): n={n:>5}  med={sub[n//2]:+.4f}  mean={sum(sub)/n:+.4f}')
