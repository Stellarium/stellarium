/* stdlib is needed for the abs function */
#include <stdlib.h>
/*
   The following prototype code was provided by Doug Tody, NRAO, for
   performing conversion between pixel arrays and line lists.  The
   compression technique is used in IRAF.
*/
int pl_p2li (int *pxsrc, int xs, short *lldst, int npix);
int pl_l2pi (short *ll_src, int xs, int *px_dst, int npix);


/*
 * PL_P2L -- Convert a pixel array to a line list.  The length of the list is
 * returned as the function value.
 *
 * Translated from the SPP version using xc -f, f2c.  8Sep99 DCT.
 */

#ifndef min
#define min(a,b)        (((a)<(b))?(a):(b))
#endif
#ifndef max
#define max(a,b)        (((a)>(b))?(a):(b))
#endif

int pl_p2li (int *pxsrc, int xs, short *lldst, int npix)
/* int *pxsrc;                      input pixel array */
/* int xs;                          starting index in pxsrc (?) */
/* short *lldst;                    encoded line list */
/* int npix;                        number of pixels to convert */
{
    /* System generated locals */
    int ret_val, i__1, i__2, i__3;

    /* Local variables */
    int zero, v, x1, hi, ip, dv, xe, np, op, iz, nv = 0, pv, nz;

    /* Parameter adjustments */
    --lldst;
    --pxsrc;

    /* Function Body */
    if (! (npix <= 0)) {
        goto L110;
    }
    ret_val = 0;
    goto L100;
L110:
    lldst[3] = -100;
    lldst[2] = 7;
    lldst[1] = 0;
    lldst[6] = 0;
    lldst[7] = 0;
    xe = xs + npix - 1;
    op = 8;
    zero = 0;
/* Computing MAX */
    i__1 = zero, i__2 = pxsrc[xs];
    pv = max(i__1,i__2);
    x1 = xs;
    iz = xs;
    hi = 1;
    i__1 = xe;
    for (ip = xs; ip <= i__1; ++ip) {
        if (! (ip < xe)) {
            goto L130;
        }
/* Computing MAX */
        i__2 = zero, i__3 = pxsrc[ip + 1];
        nv = max(i__2,i__3);
        if (! (nv == pv)) {
            goto L140;
        }
        goto L120;
L140:
        if (! (pv == 0)) {
            goto L150;
        }
        pv = nv;
        x1 = ip + 1;
        goto L120;
L150:
        goto L131;
L130:
        if (! (pv == 0)) {
            goto L160;
        }
        x1 = xe + 1;
L160:
L131:
        np = ip - x1 + 1;
        nz = x1 - iz;
        if (! (pv > 0)) {
            goto L170;
        }
        dv = pv - hi;
        if (! (dv != 0)) {
            goto L180;
        }
        hi = pv;
        if (! (abs(dv) > 4095)) {
            goto L190;
        }
        lldst[op] = (short) ((pv & 4095) + 4096);
        ++op;
        lldst[op] = (short) (pv / 4096);
        ++op;
        goto L191;
L190:
        if (! (dv < 0)) {
            goto L200;
        }
        lldst[op] = (short) (-dv + 12288);
        goto L201;
L200:
        lldst[op] = (short) (dv + 8192);
L201:
        ++op;
        if (! (np == 1 && nz == 0)) {
            goto L210;
        }
        v = lldst[op - 1];
        lldst[op - 1] = (short) (v | 16384);
        goto L91;
L210:
L191:
L180:
L170:
        if (! (nz > 0)) {
            goto L220;
        }
L230:
        if (! (nz > 0)) {
            goto L232;
        }
        lldst[op] = (short) min(4095,nz);
        ++op;
/* L231: */
        nz += -4095;
        goto L230;
L232:
        if (! (np == 1 && pv > 0)) {
            goto L240;
        }
        lldst[op - 1] = (short) (lldst[op - 1] + 20481);
        goto L91;
L240:
L220:
L250:
        if (! (np > 0)) {
            goto L252;
        }
        lldst[op] = (short) (min(4095,np) + 16384);
        ++op;
/* L251: */
        np += -4095;
        goto L250;
L252:
L91:
        x1 = ip + 1;
        iz = x1;
        pv = nv;
L120:
        ;
    }
/* L121: */
    lldst[4] = (short) ((op - 1) % 32768);
    lldst[5] = (short) ((op - 1) / 32768);
    ret_val = op - 1;
    goto L100;
L100:
    return ret_val;
} /* plp2li_ */

/*
 * PL_L2PI -- Translate a PLIO line list into an integer pixel array.
 * The number of pixels output (always npix) is returned as the function
 * value.
 *
 * Translated from the SPP version using xc -f, f2c.  8Sep99 DCT.
 */

int pl_l2pi (short *ll_src, int xs, int *px_dst, int npix)
/* short *ll_src;                   encoded line list */
/* int xs;                          starting index in ll_src */
/* int *px_dst;                    output pixel array */
/* int npix;                       number of pixels to convert */
{
    /* System generated locals */
    int ret_val, i__1, i__2;

    /* Local variables */
    int data, sw0001, otop, i__, lllen, i1, i2, x1, x2, ip, xe, np,
             op, pv, opcode, llfirt;
    int skipwd;

    /* Parameter adjustments */
    --px_dst;
    --ll_src;

    /* Function Body */
    if (! (ll_src[3] > 0)) {
        goto L110;
    }
    lllen = ll_src[3];
    llfirt = 4;
    goto L111;
L110:
    lllen = (ll_src[5] << 15) + ll_src[4];
    llfirt = ll_src[2] + 1;
L111:
    if (! (npix <= 0 || lllen <= 0)) {
        goto L120;
    }
    ret_val = 0;
    goto L100;
L120:
    xe = xs + npix - 1;
    skipwd = 0;
    op = 1;
    x1 = 1;
    pv = 1;
    i__1 = lllen;
    for (ip = llfirt; ip <= i__1; ++ip) {
        if (! skipwd) {
            goto L140;
        }
        skipwd = 0;
        goto L130;
L140:
        opcode = ll_src[ip] / 4096;
        data = ll_src[ip] & 4095;
        sw0001 = opcode;
        goto L150;
L160:
        x2 = x1 + data - 1;
        i1 = max(x1,xs);
        i2 = min(x2,xe);
        np = i2 - i1 + 1;
        if (! (np > 0)) {
            goto L170;
        }
        otop = op + np - 1;
        if (! (opcode == 4)) {
            goto L180;
        }
        i__2 = otop;
        for (i__ = op; i__ <= i__2; ++i__) {
            px_dst[i__] = pv;
/* L190: */
        }
/* L191: */
        goto L181;
L180:
        i__2 = otop;
        for (i__ = op; i__ <= i__2; ++i__) {
            px_dst[i__] = 0;
/* L200: */
        }
/* L201: */
        if (! (opcode == 5 && i2 == x2)) {
            goto L210;
        }
        px_dst[otop] = pv;
L210:
L181:
        op = otop + 1;
L170:
        x1 = x2 + 1;
        goto L151;
L220:
        pv = (ll_src[ip + 1] << 12) + data;
        skipwd = 1;
        goto L151;
L230:
        pv += data;
        goto L151;
L240:
        pv -= data;
        goto L151;
L250:
        pv += data;
        goto L91;
L260:
        pv -= data;
L91:
        if (! (x1 >= xs && x1 <= xe)) {
            goto L270;
        }
        px_dst[op] = pv;
        ++op;
L270:
        ++x1;
        goto L151;
L150:
        ++sw0001;
        if (sw0001 < 1 || sw0001 > 8) {
            goto L151;
        }
        switch ((int)sw0001) {
            case 1:  goto L160;
            case 2:  goto L220;
            case 3:  goto L230;
            case 4:  goto L240;
            case 5:  goto L160;
            case 6:  goto L160;
            case 7:  goto L250;
            case 8:  goto L260;
        }
L151:
        if (! (x1 > xe)) {
            goto L280;
        }
        goto L131;
L280:
L130:
        ;
    }
L131:
    i__1 = npix;
    for (i__ = op; i__ <= i__1; ++i__) {
        px_dst[i__] = 0;
/* L290: */
    }
/* L291: */
    ret_val = npix;
    goto L100;
L100:
    return ret_val;
} /* pll2pi_ */

