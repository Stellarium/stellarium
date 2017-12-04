/* Copyright 2013 Geehalel (geehalel AT gmail DOT com) */
/* This file is part of the Skywatcher Protocol INDI driver.

    The Skywatcher Protocol INDI driver is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    The Skywatcher Protocol INDI driver is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with the Skywatcher Protocol INDI driver.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Define vertex indices. */
#define X 0
#define Y 1
#define Z 2

/* Define structures for vertices, edges and faces */
typedef struct tVertexStructure tsVertex;
typedef tsVertex *tVertex;

typedef struct tEdgeStructure tsEdge;
typedef tsEdge *tEdge;

typedef struct tFaceStructure tsFace;
typedef tsFace *tFace;

struct tVertexStructure
{
    int v[3];
    int vnum;
    tEdge duplicate; /* pointer to incident cone edge (or NULL) */
    bool onhull;     /* T iff point on hull. */
    bool mark;       /* T iff point already processed. */
    tVertex next, prev;
};

struct tEdgeStructure
{
    tFace adjface[2];
    tVertex endpts[2];
    tFace newface; /* pointer to incident cone face. */
    bool todelete; /* T iff edge should be delete. */
    tEdge next, prev;
};

struct tFaceStructure
{
    tEdge edge[3];
    tVertex vertex[3];
    bool visible; /* T iff face visible from new point. */
    tFace next, prev;
};

/* Define flags */
#define ONHULL    TRUE
#define REMOVED   TRUE
#define VISIBLE   TRUE
#define PROCESSED TRUE
#define SAFE      1000000 /* Range of safe coord values. */

/* Global variable definitions */
extern tVertex vertices;
extern tEdge edges;
extern tFace faces;

/* Function declarations */
extern tVertex MakeNullVertex(void);
extern void ReadVertices(void);
extern void Print(void);
extern void SubVec(int a[3], int b[3], int c[3]);
extern void DoubleTriangle(void);
extern void ConstructHull(void);
extern bool AddOne(tVertex p);
extern int VolumeSign(tFace f, tVertex p);
extern int Volumei(tFace f, tVertex p);
extern tFace MakeConeFace(tEdge e, tVertex p);
extern void MakeCcw(tFace f, tEdge e, tVertex p);
extern tEdge MakeNullEdge(void);
extern tFace MakeNullFace(void);
extern tFace MakeFace(tVertex v0, tVertex v1, tVertex v2, tFace f);
extern void CleanUp(tVertex *pvnext);
extern void CleanEdges(void);
extern void CleanFaces(void);
extern void CleanVertices(tVertex *pvnext);
extern bool Collinear(tVertex a, tVertex b, tVertex c);
extern void CheckEuler(int V, int E, int F);
extern void PrintPoint(tVertex p);
extern void Checks(void);
extern void Consistency(void);
extern void Convexity(void);
extern void PrintOut(tVertex v);
extern void PrintVertices(void);
extern void PrintEdges(void);
extern void PrintFaces(void);
extern void CheckEndpts(void);
extern void EdgeOrderOnFaces(void);

#ifdef __cplusplus
}
#endif
