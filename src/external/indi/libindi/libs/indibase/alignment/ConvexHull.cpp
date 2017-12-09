/*!
 * \file ConvexHull.cpp
 *
 * \author Roger James
 * \date December 2013
 *
 */

// This C++ code is based on the c code described below
// it was ported to c++ and modded to work on on coordinate types
// other than integers by Roger James in December 2013
/*
This code is described in "Computational Geometry in C" (Second Edition),
Chapter 4.  It is not written to be comprehensible without the
explanation in that book.

Input: 3n integer coordinates for the points.
Output: the 3D convex hull, in postscript with embedded comments
        showing the vertices and faces.

Compile: gcc -o chull chull.c (or simply: make)

Written by Joseph O'Rourke, with contributions by
  Kristy Anderson, John Kutcher, Catherine Schevon, Susan Weller.
Last modified: May 2000
Questions to orourke@cs.smith.edu.

--------------------------------------------------------------------
This code is Copyright 2000 by Joseph O'Rourke.  It may be freely
redistributed in its entirety provided that this copyright notice is
not removed.
--------------------------------------------------------------------
*/

#include "ConvexHull.h"

#include <iostream>
#include <map>

namespace INDI
{
namespace AlignmentSubsystem
{
ConvexHull::ConvexHull() : vertices(nullptr), edges(nullptr), faces(nullptr), debug(false), check(false),
    ScaleFactor(SAFE-1)
{
}

bool ConvexHull::AddOne(tVertex p)
{
    tFace f;
    tEdge e, temp;
    int vol;
    bool vis = false;

    if (debug)
    {
        std::cerr << "AddOne: starting to add v" << p->vnum << ".\n";
        //PrintOut( vertices );
    }

    /* Mark faces visible from p. */
    f = faces;
    do
    {
        vol = VolumeSign(f, p);
        if (debug)
            std::cerr << "faddr: " << std::hex << f << "   paddr: " << p << "   Vol = " << std::dec << vol << '\n';
        if (vol < 0)
        {
            f->visible = VISIBLE;
            vis        = true;
        }
        f = f->next;
    } while (f != faces);

    /* If no faces are visible from p, then p is inside the hull. */
    if (!vis)
    {
        p->onhull = !ONHULL;
        return false;
    }

    /* Mark edges in interior of visible region for deletion.
    Erect a newface based on each border edge. */
    e = edges;
    do
    {
        temp = e->next;
        if (e->adjface[0]->visible && e->adjface[1]->visible)
            /* e interior: mark for deletion. */
            e->delete_it = REMOVED;
        else if (e->adjface[0]->visible || e->adjface[1]->visible)
            /* e border: make a new face. */
            e->newface = MakeConeFace(e, p);
        e = temp;
    } while (e != edges);
    return true;
}

void ConvexHull::CheckEndpts()
{
    int i;
    tFace fstart;
    tEdge e;
    tVertex v;
    bool error = false;

    fstart = faces;
    if (faces)
        do
        {
            for (i = 0; i < 3; ++i)
            {
                v = faces->vertex[i];
                e = faces->edge[i];
                if (v != e->endpts[0] && v != e->endpts[1])
                {
                    error = true;
                    std::cerr << "CheckEndpts: Error!\n";
                    std::cerr << "  addr: " << std::hex << faces << ':';
                    std::cerr << "  edges:";
                    std::cerr << "(" << e->endpts[0]->vnum << "," << e->endpts[1]->vnum << ")";
                    std::cerr << "\n";
                }
            }
            faces = faces->next;
        } while (faces != fstart);

    if (error)
        std::cerr << "Checks: ERROR found and reported above.\n";
    else
        std::cerr << "Checks: All endpts of all edges of all faces check.\n";
}

void ConvexHull::CheckEuler(int V, int E, int F)
{
    if (check)
        std::cerr << "Checks: V, E, F = " << V << ' ' << E << ' ' << F << ":\t";

    if ((V - E + F) != 2)
        std::cerr << "Checks: V-E+F != 2\n";
    else if (check)
        std::cerr << "V-E+F = 2\t";

    if (F != (2 * V - 4))
        std::cerr << "Checks: F=" << F << " != 2V-4=" << 2 * V - 4 << "; V=" << V << '\n';
    else if (check)
        std::cerr << "F = 2V-4\t";

    if ((2 * E) != (3 * F))
        std::cerr << "Checks: 2E=" << 2 * E << " != 3F=" << 3 * F << "; E=" << E << ", F=" << F << '\n';
    else if (check)
        std::cerr << "2E = 3F\n";
}

void ConvexHull::Checks()
{
    tVertex v;
    tEdge e;
    tFace f;
    int V = 0, E = 0, F = 0;

    Consistency();
    Convexity();
    if ((v = vertices) != nullptr)
        do
        {
            if (v->mark)
                V++;
            v = v->next;
        } while (v != vertices);

    if ((e = edges) != nullptr)
        do
        {
            E++;
            e = e->next;
        } while (e != edges);
    if ((f = faces) != nullptr)
        do
        {
            F++;
            f = f->next;
        } while (f != faces);
    CheckEuler(V, E, F);
    CheckEndpts();
}

void ConvexHull::CleanEdges()
{
    tEdge e; /* Primary index into edge list. */
    tEdge t; /* Temporary edge pointer. */

    /* Integrate the newface's into the data structure. */
    /* Check every edge. */
    e = edges;
    do
    {
        if (e->newface)
        {
            if (e->adjface[0]->visible)
                e->adjface[0] = e->newface;
            else
                e->adjface[1] = e->newface;
            e->newface = nullptr;
        }
        e = e->next;
    } while (e != edges);

    /* Delete any edges marked for deletion. */
    while (edges && edges->delete_it)
    {
        e = edges;
        remove<tEdge>(edges, e);
    }
    e = edges->next;
    do
    {
        if (e->delete_it)
        {
            t = e;
            e = e->next;
            remove<tEdge>(edges, t);
        }
        else
            e = e->next;
    } while (e != edges);
}

void ConvexHull::CleanFaces()
{
    tFace f; /* Primary pointer into face list. */
    tFace t; /* Temporary pointer, for deleting. */

    while (faces && faces->visible)
    {
        f = faces;
        remove<tFace>(faces, f);
    }
    f = faces->next;
    do
    {
        if (f->visible)
        {
            t = f;
            f = f->next;
            remove<tFace>(faces, t);
        }
        else
            f = f->next;
    } while (f != faces);
}

void ConvexHull::CleanUp(tVertex *pvnext)
{
    CleanEdges();
    CleanFaces();
    CleanVertices(pvnext);
}

void ConvexHull::CleanVertices(tVertex *pvnext)
{
    tEdge e;
    tVertex v, t;

    /* Mark all vertices incident to some undeleted edge as on the hull. */
    e = edges;
    do
    {
        e->endpts[0]->onhull = e->endpts[1]->onhull = ONHULL;
        e                                           = e->next;
    } while (e != edges);

    /* Delete all vertices that have been processed but
    are not on the hull. */
    while (vertices && vertices->mark && !vertices->onhull)
    {
        /* If about to delete vnext, advance it first. */
        v = vertices;
        if (v == *pvnext)
            *pvnext = v->next;
        remove<tVertex>(vertices, v);
    }
    v = vertices->next;
    do
    {
        if (v->mark && !v->onhull)
        {
            t = v;
            v = v->next;
            if (t == *pvnext)
                *pvnext = t->next;
            remove<tVertex>(vertices, t);
        }
        else
            v = v->next;
    } while (v != vertices);

    /* Reset flags. */
    v = vertices;
    do
    {
        v->duplicate = nullptr;
        v->onhull    = !ONHULL;
        v            = v->next;
    } while (v != vertices);
}

bool ConvexHull::Collinear(tVertex a, tVertex b, tVertex c)
{
    return (c->v[Z] - a->v[Z]) * (b->v[Y] - a->v[Y]) - (b->v[Z] - a->v[Z]) * (c->v[Y] - a->v[Y]) == 0 &&
           (b->v[Z] - a->v[Z]) * (c->v[X] - a->v[X]) - (b->v[X] - a->v[X]) * (c->v[Z] - a->v[Z]) == 0 &&
           (b->v[X] - a->v[X]) * (c->v[Y] - a->v[Y]) - (b->v[Y] - a->v[Y]) * (c->v[X] - a->v[X]) == 0;
}

void ConvexHull::Consistency()
{
    tEdge e;
    int i, j;

    e = edges;

    do
    {
        /* find index of endpoint[0] in adjacent face[0] */
        for (i = 0; e->adjface[0]->vertex[i] != e->endpts[0]; ++i)
            ;

        /* find index of endpoint[0] in adjacent face[1] */
        for (j = 0; e->adjface[1]->vertex[j] != e->endpts[0]; ++j)
            ;

        /* check if the endpoints occur in opposite order */
        if (!(e->adjface[0]->vertex[(i + 1) % 3] == e->adjface[1]->vertex[(j + 2) % 3] ||
              e->adjface[0]->vertex[(i + 2) % 3] == e->adjface[1]->vertex[(j + 1) % 3]))
            break;
        e = e->next;
    } while (e != edges);

    if (e != edges)
        std::cerr << "Checks: edges are NOT consistent.\n";
    else
        std::cerr << "Checks: edges consistent.\n";
}

void ConvexHull::ConstructHull()
{
    tVertex v, vnext;

    v = vertices;
    do
    {
        vnext = v->next;
        if (!v->mark)
        {
            v->mark = PROCESSED;
            AddOne(v);
            CleanUp(&vnext); /* Pass down vnext in case it gets deleted. */

            if (check)
            {
                std::cerr << "ConstructHull: After Add of " << v->vnum << " & Cleanup:\n";
                Checks();
            }
            //            if ( debug )
            //                PrintOut( v );
        }
        v = vnext;
    } while (v != vertices);
}

void ConvexHull::Convexity()
{
    tFace f;
    tVertex v;
    int vol;

    f = faces;

    do
    {
        v = vertices;
        do
        {
            if (v->mark)
            {
                vol = VolumeSign(f, v);
                if (vol < 0)
                    break;
            }
            v = v->next;
        } while (v != vertices);

        f = f->next;

    } while (f != faces);

    if (f != faces)
        std::cerr << "Checks: NOT convex.\n";
    else if (check)
        std::cerr << "Checks: convex.\n";
}

void ConvexHull::DoubleTriangle()
{
    tVertex v0, v1, v2, v3;
    tFace f0, f1 = nullptr;
    int vol;

    /* Find 3 noncollinear points. */
    v0 = vertices;
    while (Collinear(v0, v0->next, v0->next->next))
        if ((v0 = v0->next) == vertices)
        {
            std::cout << "DoubleTriangle:  All points are Collinear!\n";
            exit(0);
        }
    v1 = v0->next;
    v2 = v1->next;

    /* Mark the vertices as processed. */
    v0->mark = PROCESSED;
    v1->mark = PROCESSED;
    v2->mark = PROCESSED;

    /* Create the two "twin" faces. */
    f0 = MakeFace(v0, v1, v2, f1);
    f1 = MakeFace(v2, v1, v0, f0);

    /* Link adjacent face fields. */
    f0->edge[0]->adjface[1] = f1;
    f0->edge[1]->adjface[1] = f1;
    f0->edge[2]->adjface[1] = f1;
    f1->edge[0]->adjface[1] = f0;
    f1->edge[1]->adjface[1] = f0;
    f1->edge[2]->adjface[1] = f0;

    /* Find a fourth, noncoplanar point to form tetrahedron. */
    v3  = v2->next;
    vol = VolumeSign(f0, v3);
    while (!vol)
    {
        if ((v3 = v3->next) == v0)
        {
            std::cout << "DoubleTriangle:  All points are coplanar!\n";
            exit(0);
        }
        vol = VolumeSign(f0, v3);
    }

    /* Insure that v3 will be the first added. */
    vertices = v3;
    if (debug)
    {
        std::cerr << "DoubleTriangle: finished. Head repositioned at v3.\n";
        //PrintOut( vertices );
    }
}

void ConvexHull::EdgeOrderOnFaces()
{
    tFace f = faces;
    tEdge newEdge;
    int i, j;

    do
    {
        for (i = 0; i < 3; i++)
        {
            if (!(((f->edge[i]->endpts[0] == f->vertex[i]) && (f->edge[i]->endpts[1] == f->vertex[(i + 1) % 3])) ||
                  ((f->edge[i]->endpts[1] == f->vertex[i]) && (f->edge[i]->endpts[0] == f->vertex[(i + 1) % 3]))))
            {
                /* Change the order of the edges on the face: */
                for (j = 0; j < 3; j++)
                {
                    /* find the edge that should be there */
                    if (((f->edge[j]->endpts[0] == f->vertex[i]) &&
                         (f->edge[j]->endpts[1] == f->vertex[(i + 1) % 3])) ||
                        ((f->edge[j]->endpts[1] == f->vertex[i]) && (f->edge[j]->endpts[0] == f->vertex[(i + 1) % 3])))
                    {
                        /* Swap it with the one erroneously put into its place: */
                        if (debug)
                            std::cerr << "Making a swap in EdgeOrderOnFaces: F(" << f->vertex[0]->vnum << ','
                                      << f->vertex[1]->vnum << ',' << f->vertex[2]->vnum << "): e[" << i << "] and e[" << j
                                      << "]\n";
                        newEdge    = f->edge[i];
                        f->edge[i] = f->edge[j];
                        f->edge[j] = newEdge;
                    }
                }
            }
        }
        f = f->next;
    } while (f != faces);
}

void ConvexHull::MakeCcw(tFace f, tEdge e, tVertex p)
{
    tFace fv;          /* The visible face adjacent to e */
    int i;             /* Index of e->endpoint[0] in fv. */
    tEdge s = nullptr; /* Temporary, for swapping */

    if (e->adjface[0]->visible)
        fv = e->adjface[0];
    else
        fv = e->adjface[1];

    /* Set vertex[0] & [1] of f to have the same orientation
    as do the corresponding vertices of fv. */
    for (i = 0; fv->vertex[i] != e->endpts[0]; ++i)
        ;
    /* Orient f the same as fv. */
    if (fv->vertex[(i + 1) % 3] != e->endpts[1])
    {
        f->vertex[0] = e->endpts[1];
        f->vertex[1] = e->endpts[0];
    }
    else
    {
        f->vertex[0] = e->endpts[0];
        f->vertex[1] = e->endpts[1];
        swap<tEdge>(s, f->edge[1], f->edge[2]);
    }
    /* This swap is tricky. e is edge[0]. edge[1] is based on endpt[0],
    edge[2] on endpt[1].  So if e is oriented "forwards," we
    need to move edge[1] to follow [0], because it precedes. */

    f->vertex[2] = p;
}

ConvexHull::tFace ConvexHull::MakeConeFace(tEdge e, tVertex p)
{
    tEdge new_edge[2];
    tFace new_face;
    int i, j;

    /* Make two new edges (if don't already exist). */
    for (i = 0; i < 2; ++i)
        /* If the edge exists, copy it into new_edge. */
        if (!(new_edge[i] = e->endpts[i]->duplicate))
        {
            /* Otherwise (duplicate is nullptr), MakeNullEdge. */
            new_edge[i]             = MakeNullEdge();
            new_edge[i]->endpts[0]  = e->endpts[i];
            new_edge[i]->endpts[1]  = p;
            e->endpts[i]->duplicate = new_edge[i];
        }

    /* Make the new face. */
    new_face          = MakeNullFace();
    new_face->edge[0] = e;
    new_face->edge[1] = new_edge[0];
    new_face->edge[2] = new_edge[1];
    MakeCcw(new_face, e, p);

    /* Set the adjacent face pointers. */
    for (i = 0; i < 2; ++i)
        for (j = 0; j < 2; ++j)
            /* Only one nullptr link should be set to new_face. */
            if (!new_edge[i]->adjface[j])
            {
                new_edge[i]->adjface[j] = new_face;
                break;
            }

    return new_face;
}

ConvexHull::tFace ConvexHull::MakeFace(tVertex v0, tVertex v1, tVertex v2, tFace fold)
{
    tFace f;
    tEdge e0, e1, e2;

    /* Create edges of the initial triangle. */
    if (!fold)
    {
        e0 = MakeNullEdge();
        e1 = MakeNullEdge();
        e2 = MakeNullEdge();
    }
    else /* Copy from fold, in reverse order. */
    {
        e0 = fold->edge[2];
        e1 = fold->edge[1];
        e2 = fold->edge[0];
    }
    e0->endpts[0] = v0;
    e0->endpts[1] = v1;
    e1->endpts[0] = v1;
    e1->endpts[1] = v2;
    e2->endpts[0] = v2;
    e2->endpts[1] = v0;

    /* Create face for triangle. */
    f            = MakeNullFace();
    f->edge[0]   = e0;
    f->edge[1]   = e1;
    f->edge[2]   = e2;
    f->vertex[0] = v0;
    f->vertex[1] = v1;
    f->vertex[2] = v2;

    /* Link edges to face. */
    e0->adjface[0] = e1->adjface[0] = e2->adjface[0] = f;

    return f;
}

void ConvexHull::MakeNewVertex(double x, double y, double z, int VertexId)
{
    tVertex v;

    v       = MakeNullVertex();
    v->v[X] = x * ScaleFactor;
    v->v[Y] = y * ScaleFactor;
    v->v[Z] = z * ScaleFactor;
    v->vnum = VertexId;
    if ((std::abs(x) > SAFE) || (std::abs(y) > SAFE) || (std::abs(z) > SAFE))
    {
        std::cout << "Coordinate of vertex below might be too large: run with -d flag\n";
        PrintPoint(v);
    }
}

ConvexHull::tEdge ConvexHull::MakeNullEdge()
{
    tEdge e;

    e             = new tsEdge;
    e->adjface[0] = e->adjface[1] = e->newface = nullptr;
    e->endpts[0] = e->endpts[1] = nullptr;
    e->delete_it                = !REMOVED;
    add<tEdge>(edges, e);
    return e;
}

ConvexHull::tFace ConvexHull::MakeNullFace()
{
    tFace f;

    f = new tsFace;
    for (int i = 0; i < 3; ++i)
    {
        f->edge[i]   = nullptr;
        f->vertex[i] = nullptr;
    }
    f->visible = !VISIBLE;
    add<tFace>(faces, f);
    return f;
}

ConvexHull::tVertex ConvexHull::MakeNullVertex()
{
    tVertex v;

    v            = new tsVertex;
    v->duplicate = nullptr;
    v->onhull    = !ONHULL;
    v->mark      = !PROCESSED;
    add<tVertex>(vertices, v);

    return v;
}

void ConvexHull::Print()
{
    /* Pointers to vertices, edges, faces. */
    tVertex v;
    tEdge e;
    tFace f;
    int xmin, ymin, xmax, ymax;
    int a[3], b[3]; /* used to compute normal vector */
    /* Counters for Euler's formula. */
    int V = 0, E = 0, F = 0;
    /* Note: lowercase==pointer, uppercase==counter. */

    /*-- find X min & max --*/
    v    = vertices;
    xmin = xmax = v->v[X];
    do
    {
        if (v->v[X] > xmax)
            xmax = v->v[X];
        else if (v->v[X] < xmin)
            xmin = v->v[X];
        v = v->next;
    } while (v != vertices);

    /*-- find Y min & max --*/
    v    = vertices;
    ymin = ymax = v->v[Y];
    do
    {
        if (v->v[Y] > ymax)
            ymax = v->v[Y];
        else if (v->v[Y] < ymin)
            ymin = v->v[Y];
        v = v->next;
    } while (v != vertices);

    /* PostScript header */
    std::cout << "%!PS\n";
    std::cout << "%%BoundingBox: " << xmin << ' ' << ymin << ' ' << xmax << ' ' << ymax << '\n';
    std::cout << ".00 .00 setlinewidth\n";
    std::cout << -xmin + 72 << ' ' << -ymin + 72 << " translate\n";
    /* The +72 shifts the figure one inch from the lower left corner */

    /* Vertices. */
    v = vertices;
    do
    {
        if (v->mark)
            V++;
        v = v->next;
    } while (v != vertices);
    std::cout << "\n%% Vertices:\tV = " << V << '\n';
    std::cout << "%% index:\t\tx\ty\tz\n";
    do
    {
        std::cout << "%% " << v->vnum << ":\t" << v->v[X] << '\t' << v->v[Y] << '\t' << v->v[Z] << '\n';
        v = v->next;
    } while (v != vertices);

    /* Faces. */
    /* visible faces are printed as PS output */
    f = faces;
    do
    {
        ++F;
        f = f->next;
    } while (f != faces);
    std::cout << "\n%% Faces:\tF = " << F << '\n';
    std::cout << "%% Visible faces only: \n";
    do
    {
        /* Print face only if it is visible: if normal vector >= 0 */
        // RFJ This code is 2d so what is calculated below
        // is actually the perp product or signed area.
        SubVec(f->vertex[1]->v, f->vertex[0]->v, a);
        SubVec(f->vertex[2]->v, f->vertex[1]->v, b);
        if ((a[0] * b[1] - a[1] * b[0]) >= 0)
        {
            std::cout << "%% vnums:  " << f->vertex[0]->vnum << "  " << f->vertex[1]->vnum << "  " << f->vertex[2]->vnum
                      << '\n';
            std::cout << "newpath\n";
            std::cout << f->vertex[0]->v[X] << '\t' << f->vertex[0]->v[Y] << "\tmoveto\n";
            std::cout << f->vertex[1]->v[X] << '\t' << f->vertex[1]->v[Y] << "\tlineto\n";
            std::cout << f->vertex[2]->v[X] << '\t' << f->vertex[2]->v[Y] << "\tlineto\n";
            std::cout << "closepath stroke\n\n";
        }
        f = f->next;
    } while (f != faces);

    /* prints a list of all faces */
    std::cout << "%% List of all faces: \n";
    std::cout << "%%\tv0\tv1\tv2\t(vertex indices)\n";
    do
    {
        std::cout << "%%\t" << f->vertex[0]->vnum << '\t' << f->vertex[1]->vnum << '\t' << f->vertex[2]->vnum << '\n';
        f = f->next;
    } while (f != faces);

    /* Edges. */
    e = edges;
    do
    {
        E++;
        e = e->next;
    } while (e != edges);
    std::cout << "\n%% Edges:\tE = " << E << '\n';
    /* Edges not printed out (but easily added). */

    std::cout << "\nshowpage\n\n";

    check = true;
    CheckEuler(V, E, F);
}

void ConvexHull::PrintEdges(std::ofstream &Ofile)
{
    tEdge temp;
    int i;

    temp = edges;
    Ofile << "Edge List\n";
    if (edges)
        do
        {
            Ofile << "  addr: " << std::hex << edges << '\t';
            Ofile << "adj: ";
            for (i = 0; i < 2; ++i)
                Ofile << edges->adjface[i] << ' ';
            Ofile << " endpts:" << std::dec;
            for (i = 0; i < 2; ++i)
                Ofile << edges->endpts[i]->vnum << ' ';
            Ofile << "  del:" << edges->delete_it << '\n';
            edges = edges->next;
        } while (edges != temp);
}

void ConvexHull::PrintFaces(std::ofstream &Ofile)
{
    int i;
    tFace temp;

    temp = faces;
    Ofile << "Face List\n";
    if (faces)
        do
        {
            Ofile << "  addr: " << std::hex << faces << "  ";
            Ofile << "  edges:" << std::hex;
            for (i = 0; i < 3; ++i)
                Ofile << faces->edge[i] << ' ';
            Ofile << "  vert:" << std::dec;
            for (i = 0; i < 3; ++i)
                Ofile << ' ' << faces->vertex[i]->vnum;
            Ofile << "  vis: " << faces->visible << '\n';
            faces = faces->next;
        } while (faces != temp);
}

void ConvexHull::PrintObj(const char *FileName)
{
    tVertex v;
    tFace f;
    std::map<int, int> vnumToOffsetMap;
    int a[3], b[3]; /* used to compute normal vector */
    double c[3], length;
    std::ofstream Ofile;

    Ofile.open(FileName, std::ios_base::out | std::ios_base::trunc);

    Ofile << "# obj file written by chull\n";
    Ofile << "mtllib chull.mtl\n";
    Ofile << "g Object001\n";
    Ofile << "s 1\n";
    Ofile << "usemtl default\n";

    // The convex hull code removes vertices from the list that at are on the hull
    // so the vertices list might have missing vnums. So I need to construct a map of
    // vnums to new vertex array indices.
    int Offset = 1;
    v          = vertices;
    do
    {
        vnumToOffsetMap[v->vnum] = Offset;
        Ofile << "v " << v->v[X] << ' ' << v->v[Y] << ' ' << v->v[Z] << '\n';
        Offset++;
        v = v->next;
    } while (v != vertices);

    // normals
    f = faces;
    do
    {
        // get two tangent vectors
        SubVec(f->vertex[1]->v, f->vertex[0]->v, a);
        //        SubVec( f->vertex[2]->v, f->vertex[1]->v, b );
        SubVec(f->vertex[2]->v, f->vertex[0]->v, b);
        // cross product for the normal
        c[0] = a[1] * b[2] - a[2] * b[1];
        c[1] = a[2] * b[0] - a[0] * b[2];
        c[2] = a[0] * b[1] - a[1] * b[0];
        // normalise
        length = sqrt((c[0] * c[0]) + (c[1] * c[1]) + (c[2] * c[2]));
        c[0]   = c[0] / length;
        c[1]   = c[1] / length;
        c[2]   = c[2] / length;
        Ofile << "vn " << c[0] << ' ' << c[1] << ' ' << c[2] << '\n';
        f = f->next;
    } while (f != faces);

    // Faces
    int i = 1;
    f     = faces;
    do
    {
        Ofile << "f " << vnumToOffsetMap[f->vertex[0]->vnum] << "//" << i << ' ' << vnumToOffsetMap[f->vertex[1]->vnum]
              << "//" << i << ' ' << vnumToOffsetMap[f->vertex[2]->vnum] << "//" << i << '\n';
        i++;
        f = f->next;
    } while (f != faces);

    Ofile.close();

    Ofile.open("chull.mtl", std::ios_base::out | std::ios_base::trunc);

    Ofile << "newmtl default\n";
    Ofile << "Ka 0.2 0 0\n";
    Ofile << "Kd 0.8 0 0\n";
    Ofile << "illum 1\n";

    Ofile.close();
}

void ConvexHull::PrintOut(const char *FileName, tVertex v)
{
    std::ofstream Ofile;
    Ofile.open(FileName, std::ios_base::out | std::ios_base::trunc);

    Ofile << "\nHead vertex " << v->vnum << " = " << std::hex << v << " :\n";

    PrintVertices(Ofile);
    PrintEdges(Ofile);
    PrintFaces(Ofile);

    Ofile.close();
}

void ConvexHull::PrintPoint(tVertex p)
{
    for (int i = 0; i < 3; i++)
        std::cout << '\t' << p->v[i];

    std::cout << '\n';
}

void ConvexHull::PrintVertices(std::ofstream &Ofile)
{
    tVertex temp;

    temp = vertices;
    Ofile << "Vertex List\n";
    if (vertices)
        do
        {
            Ofile << "  addr " << std::hex << vertices << "\t";
            Ofile << "  vnum " << std::dec << vertices->vnum;
            Ofile << '(' << vertices->v[X] << ',' << vertices->v[Y] << ',' << vertices->v[Z] << ')';
            Ofile << "  active:" << vertices->onhull;
            Ofile << "  dup:" << std::hex << vertices->duplicate;
            Ofile << "  mark:" << std::dec << vertices->mark << '\n';
            vertices = vertices->next;
        } while (vertices != temp);
}

void ConvexHull::ReadVertices()
{
    tVertex v;
    int x, y, z;
    int vnum = 0;

    while (!(std::cin.eof() || std::cin.fail()))
    {
        std::cin >> x >> y >> z;
        v       = MakeNullVertex();
        v->v[X] = x;
        v->v[Y] = y;
        v->v[Z] = z;
        v->vnum = vnum++;
        if ((abs(x) > SAFE) || (abs(y) > SAFE) || (abs(z) > SAFE))
        {
            std::cout << "Coordinate of vertex below might be too large: run with -d flag\n";
            PrintPoint(v);
        }
    }
}

void ConvexHull::Reset()
{
    tVertex CurrentVertex = vertices;
    tEdge CurrentEdge     = edges;
    tFace CurrentFace     = faces;

    if (nullptr != CurrentVertex)
    {
        do
        {
            tVertex TempVertex = CurrentVertex;
            CurrentVertex      = CurrentVertex->next;
            delete TempVertex;
        } while (CurrentVertex != vertices);
        vertices = nullptr;
    }

    if (nullptr != CurrentEdge)
    {
        do
        {
            tEdge TempEdge = CurrentEdge;
            CurrentEdge    = CurrentEdge->next;
            delete TempEdge;
        } while (CurrentEdge != edges);
        edges = nullptr;
    }

    if (nullptr != CurrentFace)
    {
        do
        {
            tFace TempFace = CurrentFace;
            CurrentFace    = CurrentFace->next;
            delete TempFace;
        } while (CurrentFace != faces);
        faces = nullptr;
    }

    debug = false;
    check = false;
}

void ConvexHull::SubVec(int a[3], int b[3], int c[3])
{
    int i;

    for (i = 0; i < 3; i++) // RFJ
        //for( i=0; i < 2; i++ )
        c[i] = a[i] - b[i];
}

int ConvexHull::Volumei(tFace f, tVertex p)
{
    int vol;
    int ax, ay, az, bx, by, bz, cx, cy, cz;

    ax = f->vertex[0]->v[X] - p->v[X];
    ay = f->vertex[0]->v[Y] - p->v[Y];
    az = f->vertex[0]->v[Z] - p->v[Z];
    bx = f->vertex[1]->v[X] - p->v[X];
    by = f->vertex[1]->v[Y] - p->v[Y];
    bz = f->vertex[1]->v[Z] - p->v[Z];
    cx = f->vertex[2]->v[X] - p->v[X];
    cy = f->vertex[2]->v[Y] - p->v[Y];
    cz = f->vertex[2]->v[Z] - p->v[Z];

    vol = (ax * (by * cz - bz * cy) + ay * (bz * cx - bx * cz) + az * (bx * cy - by * cx));

    return vol;
}

int ConvexHull::VolumeSign(tFace f, tVertex p)
{
    double vol;
    int voli;
    double ax, ay, az, bx, by, bz, cx, cy, cz;

    ax = f->vertex[0]->v[X] - p->v[X];
    ay = f->vertex[0]->v[Y] - p->v[Y];
    az = f->vertex[0]->v[Z] - p->v[Z];
    bx = f->vertex[1]->v[X] - p->v[X];
    by = f->vertex[1]->v[Y] - p->v[Y];
    bz = f->vertex[1]->v[Z] - p->v[Z];
    cx = f->vertex[2]->v[X] - p->v[X];
    cy = f->vertex[2]->v[Y] - p->v[Y];
    cz = f->vertex[2]->v[Z] - p->v[Z];

    vol = ax * (by * cz - bz * cy) + ay * (bz * cx - bx * cz) + az * (bx * cy - by * cx);

    if (debug)
    {
        /* Compute the volume using integers for comparison. */
        voli = Volumei(f, p);
        std::cerr << "Face=" << std::hex << f << "; Vertex=" << std::dec << p->vnum << ": vol(int) = " << voli
                  << ", vol(double) = " << vol << "\n";
    }

    /* The volume should be an integer. */
    if (vol > 0.5)
        return 1;
    else if (vol < -0.5)
        return -1;
    else
        return 0;
}

} // namespace AlignmentSubsystem
} // namespace INDI
