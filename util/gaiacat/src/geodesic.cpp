// Geodesic grid zone computation — matches StelGeodesicGrid / Python GeodesicGrid

#include "geodesic.hpp"
#include <cmath>
#include <array>

namespace {

constexpr double PHI  = 1.6180339887498948482;   // (1+sqrt(5))/2
constexpr double NORM = 1.9021130325903071442;   // sqrt(1+PHI*PHI)
constexpr double _a   = PHI / NORM;
constexpr double _b   = 1.0 / NORM;

// Icosahedron vertices × 12
constexpr double ICOS_VERTICES[12][3] = {
	{ _a, -_b,  0.0}, { _a,  _b,  0.0}, {-_a,  _b,  0.0}, {-_a, -_b,  0.0},
	{0.0,  _a, -_b},  {0.0,  _a,  _b},  {0.0, -_a,  _b},  {0.0, -_a, -_b},
	{-_b, 0.0,  _a},  { _b, 0.0,  _a},  { _b, 0.0, -_a},  {-_b, 0.0, -_a},
};

// Icosahedron faces × 20, each face = 3 vertex indices
constexpr int ICOS_FACES[20][3] = {
	{1,0,10},{0,1,9},{0,9,6},{9,8,6},{0,7,10},{6,7,0},{7,6,3},{6,8,3},
	{11,10,7},{7,3,11},{3,2,11},{2,3,8},{10,11,4},{2,4,11},{5,4,2},{2,8,5},
	{4,1,10},{4,5,1},{5,9,1},{8,9,5},
};

inline void cross(const double a[3], const double b[3], double out[3]) {
	out[0] = a[1]*b[2] - a[2]*b[1];
	out[1] = a[2]*b[0] - a[0]*b[2];
	out[2] = a[0]*b[1] - a[1]*b[0];
}

inline double dot(const double a[3], const double b[3]) {
	return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}

inline void normalize(double v[3]) {
	double len = std::sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
	if (len > 0) { v[0] /= len; v[1] /= len; v[2] /= len; }
}

void mid_edge_norm(const double a[3], const double b[3], double out[3]) {
	out[0] = a[0] + b[0];
	out[1] = a[1] + b[1];
	out[2] = a[2] + b[2];
	normalize(out);
}

void corners(int level, int index, double c0[3], double c1[3], double c2[3]) {
	if (level == 0) {
		int f = index;
		for (int j = 0; j < 3; ++j) {
			c0[j] = ICOS_VERTICES[ICOS_FACES[f][0]][j];
			c1[j] = ICOS_VERTICES[ICOS_FACES[f][1]][j];
			c2[j] = ICOS_VERTICES[ICOS_FACES[f][2]][j];
		}
	} else {
		int parent_lev = level - 1;
		int parent_idx = index >> 2;
		double p0[3], p1[3], p2[3];
		corners(parent_lev, parent_idx, p0, p1, p2);

		double e0[3], e1[3], e2[3];
		mid_edge_norm(p1, p2, e0);
		mid_edge_norm(p2, p0, e1);
		mid_edge_norm(p0, p1, e2);

		int s = index & 3;
		if (s == 0)      { for(int j=0;j<3;++j){c0[j]=p0[j];c1[j]=e2[j];c2[j]=e1[j];} }
		else if (s == 1) { for(int j=0;j<3;++j){c0[j]=e2[j];c1[j]=p1[j];c2[j]=e0[j];} }
		else if (s == 2) { for(int j=0;j<3;++j){c0[j]=e1[j];c1[j]=e0[j];c2[j]=p2[j];} }
		else             { for(int j=0;j<3;++j){c0[j]=e0[j];c1[j]=e1[j];c2[j]=e2[j];} }
	}
}

} // anonymous namespace

int zone_number(double x, double y, double z, int level) {
	const double pt[3] = {x, y, z};

	// Find which icosahedron face contains the point
	for (int i = 0; i < 20; ++i) {
		const double* v0 = ICOS_VERTICES[ICOS_FACES[i][0]];
		const double* v1 = ICOS_VERTICES[ICOS_FACES[i][1]];
		const double* v2 = ICOS_VERTICES[ICOS_FACES[i][2]];

		double c01[3], c12[3], c20[3];
		cross(v0, v1, c01);
		cross(v1, v2, c12);
		cross(v2, v0, c20);

		if (dot(c01, pt) >= -1e-10 && dot(c12, pt) >= -1e-10 && dot(c20, pt) >= -1e-10) {
			int idx = i;
			for (int lev = 0; lev < level; ++lev) {
				double c0[3], c1[3], c2[3];
				corners(lev, idx, c0, c1, c2);

				double e0[3], e1[3], e2[3];
				mid_edge_norm(c1, c2, e0);
				mid_edge_norm(c2, c0, e1);
				mid_edge_norm(c0, c1, e2);

				double ce1e2[3], ce2e0[3], ce0e1[3];
				cross(e1, e2, ce1e2);
				cross(e2, e0, ce2e0);
				cross(e0, e1, ce0e1);

				idx <<= 2;
				if      (dot(ce1e2, pt) <= 1e-10) { /* 0 */ }
				else if (dot(ce2e0, pt) <= 1e-10) { idx += 1; }
				else if (dot(ce0e1, pt) <= 1e-10) { idx += 2; }
				else                               { idx += 3; }
			}
			return idx;
		}
	}
	return 0;
}
