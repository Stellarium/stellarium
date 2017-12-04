/* This code is derived from the Hierarchical Triangular Mesh package found in
http://skyserver.org/htm/src/cpp/htmIndex.tar.gz
*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif
#define IDSIZE 64

typedef double float64;

#ifdef _WIN32
typedef __int64 int64;
typedef unsigned __int64 uint64;

typedef __int32 int32;
typedef unsigned __int32 uint32;
#else
typedef long long int64;
typedef unsigned long long uint64;

typedef long int32;
typedef unsigned long uint32;
#define IDHIGHBIT  0x8000000000000000LL
#define IDHIGHBIT2 0x4000000000000000LL
#endif

#define HTMNAMEMAX 32

#define prmag(lab, v)                                           \
    {                                                           \
        double prtmp = v[0] * v[0] + v[1] * v[1] + v[2] * v[2]; \
        if (prtmp > 1 + gEpsilon || prtmp < 1 - gEpsilon)       \
            printf("%s: Mag^2 = %f\n", lab, prtmp);             \
    }
#define m4_midpoint(v1, v2, w, tmp)                           \
    {                                                         \
        w[0] = v1[0] + v2[0];                                 \
        w[1] = v1[1] + v2[1];                                 \
        w[2] = v1[2] + v2[2];                                 \
        tmp  = sqrt(w[0] * w[0] + w[1] * w[1] + w[2] * w[2]); \
        w[0] /= tmp;                                          \
        w[1] /= tmp;                                          \
        w[2] /= tmp;                                          \
    }

#define copy_vec(d, s) \
    {                  \
        d[0] = s[0];   \
        d[1] = s[1];   \
        d[2] = s[2];   \
    }

static const double gEpsilon = 1.0E-15;
// const float64 gEpsilon = 1.0E-15;

const double cc_Pi = 3.1415926535897932385E0;
const double cc_Pr = 3.1415926535897932385E0 / 180.0;

typedef unsigned long long HtmID;
typedef char HtmName[HTMNAMEMAX];

uint64 cc_name2ID(const char *name);
int cc_ID2name(char *name, uint64 id);
int cc_isinside(double *p, double *v1, double *v2, double *v3);
int cc_startpane(double *v1, double *v2, double *v3, double xin, double yin, double zin, char *name);
int cc_parseVectors(char *spec, int *level, double *ra, double *dec);
uint64 cc_vector2ID(double x, double y, double z, int depth);
uint64 cc_radec2ID(double ra, double dec, int depth);
/* int cc_esolve(double *v1, double *v2,
		double ax, double ay, double az, double d);*/

#ifdef __cplusplus
}
#endif
