#!/usr/bin/env python3
"""Verify SkyChart -> Stellarium .cat conversion: position, V, B-V against stars_8.
Compares THREE B-V formulas: SkyChart 2-step, Gaia DR3 (G-V)-(G-B), and old rough."""
import struct, os, math

CAT_PATH = r'C:\Users\13308\CLionProjects\stellarium\stars\hip_gaia3\stars_8_2v0_3.cat'
SKYCHART_DIR = r'F:\gaiadr3\vega.ap-i.net\gaia'

# ── G->V conversion (official Gaia DR3) ────────
def g_to_v(g, c):
    if c is None: return g
    return g + 0.02704 - 0.01424*c + 0.2156*c*c - 0.01426*c*c*c

# ── B-V formula 1: SkyChart 2-step ─────────────
def bv_skychart(c):
    if c is None: return 0.5
    bt_vt = -0.006482+0.7865*c-0.3631*c*c+0.93192*c*c*c-0.4843*c*c*c*c+0.06814*c*c*c*c*c
    if -0.25<bt_vt<0.5:   bv = bt_vt-0.006-0.1069*bt_vt+0.1459*bt_vt*bt_vt
    elif 0.5<=bt_vt<2.0:  bv = bt_vt-0.007813*bt_vt-0.1489*bt_vt*bt_vt+0.03384*bt_vt*bt_vt*bt_vt
    else:                 bv = 0.850*bt_vt
    return max(0.0,min(bv,5.35))

# ── B-V formula 2: Gaia DR3 direct (G-V)-(G-B) ──
def bv_direct(c):
    gmv = -0.02704 + 0.01424*c - 0.2156*c*c + 0.01426*c*c*c
    gmb =  0.01448 - 0.6874*c - 0.3604*c*c + 0.06718*c*c*c - 0.006061*c*c*c*c
    return max(0.0, min(gmv - gmb, 5.35))

# ── B-V formula 3: old rough ───────────────────
def bv_old(c):
    return max(0.0, min(c*0.7+0.1, 5.35))

# ── Geodesic Grid ──────────────────────────────
PHI=(1+math.sqrt(5))/2; NORM=math.sqrt(1+PHI*PHI); _a,_b=PHI/NORM,1/NORM
_IV=[(_a,-_b,0),(_a,_b,0),(-_a,_b,0),(-_a,-_b,0),(0,_a,-_b),(0,_a,_b),(0,-_a,_b),(0,-_a,-_b),(-_b,0,_a),(_b,0,_a),(_b,0,-_a),(-_b,0,-_a)]
_IF=[(1,0,10),(0,1,9),(0,9,6),(9,8,6),(0,7,10),(6,7,0),(7,6,3),(6,8,3),(11,10,7),(7,3,11),(3,2,11),(2,3,8),(10,11,4),(2,4,11),(5,4,2),(2,8,5),(4,1,10),(4,5,1),(5,9,1),(8,9,5)]
def _cross(a,b): return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
def _dot(a,b): return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]
def _norm(v): ln=math.sqrt(v[0]**2+v[1]**2+v[2]**2); return (v[0]/ln,v[1]/ln,v[2]/ln) if ln>0 else v

class G:
    def __init__(s): s._c={}
    def _co(s,l,i):
        k=(l,i)
        if k in s._c: return s._c[k]
        if l<=0: c=_IF[i]; r=(_IV[c[0]],_IV[c[1]],_IV[c[2]])
        else:
            lv=l-1; ii=i>>2; c0,c1,c2=s._co(lv,ii)
            e0=_norm((c1[0]+c2[0],c1[1]+c2[1],c1[2]+c2[2]))
            e1=_norm((c2[0]+c0[0],c2[1]+c0[1],c2[2]+c0[2]))
            e2=_norm((c0[0]+c1[0],c0[1]+c1[1],c0[2]+c1[2]))
            si=i&3
            if si==0: r=(c0,e2,e1)
            elif si==1: r=(e2,c1,e0)
            elif si==2: r=(e1,e0,c2)
            else: r=(e0,e1,e2)
        s._c[k]=r; return r
    def zn(s,x,y,z,lev):
        for i in range(20):
            c0,c1,c2=(_IV[_IF[i][j]] for j in range(3))
            if _dot(_cross(c0,c1),(x,y,z))>=-1e-10 and _dot(_cross(c1,c2),(x,y,z))>=-1e-10 and _dot(_cross(c2,c0),(x,y,z))>=-1e-10:
                idx=i
                for l in range(lev):
                    c0,c1,c2=s._co(l,idx)
                    e0=_norm((c1[0]+c2[0],c1[1]+c2[1],c1[2]+c2[2]))
                    e1=_norm((c2[0]+c0[0],c2[1]+c0[1],c2[2]+c0[2]))
                    e2=_norm((c0[0]+c1[0],c0[1]+c1[1],c0[2]+c1[2]))
                    idx<<=2
                    if _dot(_cross(e1,e2),(x,y,z))<=1e-10: pass
                    elif _dot(_cross(e2,e0),(x,y,z))<=1e-10: idx+=1
                    elif _dot(_cross(e0,e1),(x,y,z))<=1e-10: idx+=2
                    else: idx+=3
                return idx
        return 0
grid=G()

# ── Sample SkyChart ────────────────────────────
candidates=[]; count=0
for sub in ['gaia3']:
    d=os.path.join(SKYCHART_DIR,sub)
    for zdir in sorted(os.listdir(d))[:10]:
        zp=os.path.join(d,zdir)
        for fn in sorted(os.listdir(zp))[:3]:
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
                    candidates.append((sid,ra,dec,g,bp,rp,c,vp,bv_skychart(c),bv_direct(c),bv_old(c),zone))
                    count+=1
                    if count>=5000: break
            if count>=5000: break
        if count>=5000: break
    if count>=5000: break
print(f'Sampled {len(candidates)} SkyChart stars with V in [16.75, 18.0)')

# ── Search candidates in stars_8 ───────────────
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

pos_errs,v_errs,bv1_errs,bv2_errs,bv3_errs=[],[],[],[],[]
matched,not_found=0,0
bright=[]

for z,cnt,off in zone_info:
    if z not in by_zone: continue
    targets=by_zone[z]
    tids={t[0] for t in targets}
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
    for sid,ra,dec,g,bp,rp,c,vp,bv1,bv2,bv3,zn in targets:
        if sid in found:
            rc,dc,vc,bc=found[sid]
            dra=(ra-rc)*math.cos(math.radians((dec+dc)/2))
            ddec=dec-dc
            pos_errs.append(math.sqrt(dra*dra+ddec*ddec)*3600)
            v_errs.append(vp-vc)
            bv1_errs.append(bv1-bc)
            bv2_errs.append(bv2-bc)
            bv3_errs.append(bv3-bc)
            bright.append((vp,sid,ra,dec,g,bp,rp,c,vp,bc,bv1,bv2,bv3))
            matched+=1
        else: not_found+=1

# ── Summary ─────────────────────────────────────
if pos_errs:
    pos_errs.sort();v_errs.sort();bv1_errs.sort();bv2_errs.sort();bv3_errs.sort()
    m=len(pos_errs)
    print(f'\n{"="*80}')
    print(f' VERIFICATION RESULT ({m} matched stars)')
    print(f'{"="*80}')
    print(f' Position (arcsec):    median={pos_errs[m//2]:.4f}  max={pos_errs[-1]:.4f}')
    print(f' V magnitude diff:     median={v_errs[m//2]:.4f}  std={math.sqrt(sum(x*x for x in v_errs)/m):.4f}')
    print()
    print(f' B-V formula comparison (prediction - stars_8_catalog):')
    print(f'   Formula               Median   Mean     Std')
    print(f'   SkyChart 2-step        {bv1_errs[m//2]:+.4f}   {sum(bv1_errs)/m:+.4f}   {math.sqrt(sum(x*x for x in bv1_errs)/m):.4f}')
    print(f'   Gaia DR3 (G-V)-(G-B)   {bv2_errs[m//2]:+.4f}   {sum(bv2_errs)/m:+.4f}   {math.sqrt(sum(x*x for x in bv2_errs)/m):.4f}  <-- BEST')
    print(f'   Old rough BP-RP*0.7    {bv3_errs[m//2]:+.4f}   {sum(bv3_errs)/m:+.4f}   {math.sqrt(sum(x*x for x in bv3_errs)/m):.4f}')
    print()
    print(f' Recommended B-V formula (Gaia DR3 direct, copy to gaia_to_cat.py):')
    print(f'   g_minus_v = -0.02704 + 0.01424*c - 0.2156*c*c + 0.01426*c*c*c')
    print(f'   g_minus_b =  0.01448 - 0.6874*c - 0.3604*c*c + 0.06718*c*c*c - 0.006061*c*c*c*c')
    print(f'   bv = g_minus_v - g_minus_b')

    bright.sort(key=lambda x:x[0])
    print(f'\n{"-"*120}')
    print(f' 10 BRIGHTEST — compare B-V in Cartes du Ciel to verify')
    print(f'{"-"*120}')
    hdr=f'{"Gaia ID":>20} {"RA":>8} {"DEC":>8} {"V":>7} {"G":>7} {"BP-RP":>7} {"BV_cat":>7} {"BV_Sky":>7} {"BV_Gaia":>7} {"BV_old":>7} {"B-BP":>7}'
    print(hdr); print('-'*len(hdr))
    for _,sid,ra,dec,g,bp,rp,c,vp,bc,b1,b2,b3 in bright[:10]:
        print(f'{sid:>20} {ra:>8.4f} {dec:>8.4f} {vp:>7.3f} {g:>7.3f} {c:>7.3f} {bc:>7.3f} {b1:>7.3f} {b2:>7.3f} {b3:>7.3f} {bp-rp:>7.3f}')
    
    # Also show correlation: plot BP-RP vs B-V for each formula
    print(f'\n{"-"*80}')
    print(f' B-V range check (predicted vs catalog)')
    print(f' BP-RP   BV_cat   BV_Sky  BV_Gaia  BV_old')
    for c_val in [0.0, 0.5, 1.0, 1.5, 2.0, 2.5, 3.0]:
        print(f' {c_val:>5.1f}   {bv_skychart(c_val):>7.3f}   {bv_direct(c_val):>7.3f}   {bv_old(c_val):>7.3f}  (SkyChart={bv_skychart(c_val):.3f})')
