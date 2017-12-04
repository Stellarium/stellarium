/*  This file, swapproc.c, contains general utility routines that are      */
/*  used by other FITSIO routines to swap bytes.                           */

/*  The FITSIO software was written by William Pence at the High Energy    */
/*  Astrophysic Science Archive Research Center (HEASARC) at the NASA      */
/*  Goddard Space Flight Center.                                           */

/* The fast SSE2 and SSSE3 functions were provided by Julian Taylor, ESO */

#include <string.h>
#include <stdlib.h>
#include "fitsio2.h"

/* bswap builtin is available since GCC 4.3 */
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)
#define HAVE_BSWAP
#endif

#ifdef __SSSE3__
#include <tmmintrin.h>
/* swap 16 bytes according to mask, values must be 16 byte aligned */
static inline void swap_ssse3(char * values, __m128i mask)
{
    __m128i v = _mm_load_si128((__m128i *)values);
    __m128i s = _mm_shuffle_epi8(v, mask);
    _mm_store_si128((__m128i*)values, s);
}
#endif
#ifdef __SSE2__
#include <emmintrin.h>
/* swap 8 shorts, values must be 16 byte aligned
 * faster than ssse3 variant for shorts */
static inline void swap2_sse2(char * values)
{
    __m128i r1 = _mm_load_si128((__m128i *)values);
    __m128i r2 = r1;
    r1 = _mm_srli_epi16(r1, 8);
    r2 = _mm_slli_epi16(r2, 8);
    r1 = _mm_or_si128(r1, r2);
    _mm_store_si128((__m128i*)values, r1);
}
/* the three shuffles required for 4 and 8 byte variants make
 * SSE2 slower than bswap */


/* get number of elements to peel to reach alignment */
static inline size_t get_peel(void * addr, size_t esize, size_t nvals,
                              size_t alignment)
{
    const size_t offset = (size_t)addr % alignment;
    size_t peel = offset ? (alignment - offset) / esize : 0;
    peel = nvals < peel ? nvals : peel;
    return peel;
}
#endif

/*--------------------------------------------------------------------------*/
static void ffswap2_slow(short *svalues, long nvals)
{
    register long ii;
    unsigned short * usvalues;

    usvalues = (unsigned short *) svalues;

    for (ii = 0; ii < nvals; ii++)
    {
        usvalues[ii] = (usvalues[ii]>>8) | (usvalues[ii]<<8);
    }
}
/*--------------------------------------------------------------------------*/
#if __SSE2__
void ffswap2(short *svalues,  /* IO - pointer to shorts to be swapped    */
             long nvals)     /* I  - number of shorts to be swapped     */
/*
  swap the bytes in the input short integers: ( 0 1 -> 1 0 )
*/
{
    if ((long)svalues % 2 != 0) { /* should not happen */
        ffswap2_slow(svalues, nvals);
        return;
    }

    long ii;
    size_t peel = get_peel((void*)&svalues[0], sizeof(svalues[0]), nvals, 16);

    ffswap2_slow(svalues, peel);
    for (ii = peel; ii < (nvals - peel - (nvals - peel) % 8); ii+=8) {
        swap2_sse2((char*)&svalues[ii]);
    }
    ffswap2_slow(&svalues[ii], nvals - ii);
}
#else
void ffswap2(short *svalues,  /* IO - pointer to shorts to be swapped    */
             long nvals)     /* I  - number of shorts to be swapped     */
/*
  swap the bytes in the input 4-byte integer: ( 0 1 2 3 -> 3 2 1 0 )
*/
{
    ffswap2_slow(svalues, nvals);
}
#endif
/*--------------------------------------------------------------------------*/
static void ffswap4_slow(INT32BIT *ivalues, long nvals)
{
    register long ii;

#if defined(HAVE_BSWAP)
    for (ii = 0; ii < nvals; ii++)
    {
        ivalues[ii] = __builtin_bswap32(ivalues[ii]);
    }
#elif defined(_MSC_VER) && (_MSC_VER >= 1400)
    /* intrinsic byte swapping function in Microsoft Visual C++ 8.0 and later */
    unsigned int* uivalues = (unsigned int *) ivalues;

    /* intrinsic byte swapping function in Microsoft Visual C++ */
    for (ii = 0; ii < nvals; ii++)
    {
        uivalues[ii] = _byteswap_ulong(uivalues[ii]);
    }
#else
    char *cvalues, tmp;

    for (ii = 0; ii < nvals; ii++)
    {
        cvalues = (char *)&ivalues[ii];
        tmp = cvalues[0];
        cvalues[0] = cvalues[3];
        cvalues[3] = tmp;
        tmp = cvalues[1];
        cvalues[1] = cvalues[2];
        cvalues[2] = tmp;
    }
#endif
}
/*--------------------------------------------------------------------------*/
#ifdef __SSSE3__
void ffswap4(INT32BIT *ivalues,  /* IO - pointer to INT*4 to be swapped    */
                 long nvals)     /* I  - number of floats to be swapped     */
/*
  swap the bytes in the input 4-byte integer: ( 0 1 2 3 -> 3 2 1 0 )
*/
{
    if ((long)ivalues % 4 != 0) { /* should not happen */
        ffswap4_slow(ivalues, nvals);
        return;
    }

    long ii;
    const __m128i cmask4 = _mm_set_epi8(12, 13, 14, 15,
                                        8, 9, 10, 11,
                                        4, 5, 6, 7,
                                        0, 1, 2 ,3);
    size_t peel = get_peel((void*)&ivalues[0], sizeof(ivalues[0]), nvals, 16);
    ffswap4_slow(ivalues, peel);
    for (ii = peel; ii < (nvals - peel - (nvals - peel) % 4); ii+=4) {
        swap_ssse3((char*)&ivalues[ii], cmask4);
    }
    ffswap4_slow(&ivalues[ii], nvals - ii);
}
#else
void ffswap4(INT32BIT *ivalues,  /* IO - pointer to INT*4 to be swapped    */
                 long nvals)     /* I  - number of floats to be swapped     */
/*
  swap the bytes in the input 4-byte integer: ( 0 1 2 3 -> 3 2 1 0 )
*/
{
    ffswap4_slow(ivalues, nvals);
}
#endif
/*--------------------------------------------------------------------------*/
static void ffswap8_slow(double *dvalues, long nvals)
{
    register long ii;
#ifdef HAVE_BSWAP
    LONGLONG * llvalues = (LONGLONG*)dvalues;

    for (ii = 0; ii < nvals; ii++) {
        llvalues[ii] = __builtin_bswap64(llvalues[ii]);
    }
#elif defined(_MSC_VER) && (_MSC_VER >= 1400)
    /* intrinsic byte swapping function in Microsoft Visual C++ 8.0 and later */
    unsigned __int64 * llvalues = (unsigned __int64 *) dvalues;

    for (ii = 0; ii < nvals; ii++)
    {
        llvalues[ii] = _byteswap_uint64(llvalues[ii]);
    }
#else
    register char *cvalues;
    register char temp;

    cvalues = (char *) dvalues;      /* copy the pointer value */

    for (ii = 0; ii < nvals*8; ii += 8)
    {
        temp = cvalues[ii];
        cvalues[ii] = cvalues[ii+7];
        cvalues[ii+7] = temp;

        temp = cvalues[ii+1];
        cvalues[ii+1] = cvalues[ii+6];
        cvalues[ii+6] = temp;

        temp = cvalues[ii+2];
        cvalues[ii+2] = cvalues[ii+5];
        cvalues[ii+5] = temp;

        temp = cvalues[ii+3];
        cvalues[ii+3] = cvalues[ii+4];
        cvalues[ii+4] = temp;
    }
#endif
}
/*--------------------------------------------------------------------------*/
#ifdef __SSSE3__
void ffswap8(double *dvalues,  /* IO - pointer to doubles to be swapped     */
             long nvals)       /* I  - number of doubles to be swapped      */
/*
  swap the bytes in the input doubles: ( 01234567  -> 76543210 )
*/
{
    if ((long)dvalues % 8 != 0) { /* should not happen on amd64 */
        ffswap8_slow(dvalues, nvals);
        return;
    }

    long ii;
    const __m128i cmask8 = _mm_set_epi8(8, 9, 10, 11, 12, 13, 14, 15,
                                        0, 1, 2 ,3, 4, 5, 6, 7);
    size_t peel = get_peel((void*)&dvalues[0], sizeof(dvalues[0]), nvals, 16);
    ffswap8_slow(dvalues, peel);
    for (ii = peel; ii < (nvals - peel - (nvals - peel) % 2); ii+=2) {
        swap_ssse3((char*)&dvalues[ii], cmask8);
    }
    ffswap8_slow(&dvalues[ii], nvals - ii);
}
#else
void ffswap8(double *dvalues,  /* IO - pointer to doubles to be swapped     */
             long nvals)       /* I  - number of doubles to be swapped      */
/*
  swap the bytes in the input doubles: ( 01234567  -> 76543210 )
*/
{
    ffswap8_slow(dvalues, nvals);
}
#endif
