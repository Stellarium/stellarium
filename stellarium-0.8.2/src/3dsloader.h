#ifndef _3DSLOADER
#define _3DSLOADER

#include <vector>
using namespace std;

/*
* ---------------- www.spacesimulator.net --------------
*   ---- Space simulators and 3d engine tutorials ----
*
*  Author: Damiano Vitulli <info@spacesimulator.net>
*
* ALL RIGHTS RESERVED
*
*
* Tutorial 4: 3d engine - 3ds models loader
* 
* File header: 3dsloader.h
*  
*/

/**********************************************************
*
* TYPES DECLARATION
*
*********************************************************/

#define MAX_VERTICES 100000 // Max number of vertices (for each object)
#define MAX_POLYGONS 100000 // Max number of polygons (for each object)

// Our vertex type
typedef struct{
  double x,y,z;
}vertex_type;

// The polygon (triangle), 3 numbers that aim 3 vertices
typedef struct{
  unsigned short a,b,c;
}polygon_type;

// The mapcoord type, 2 texture coordinates for each vertex
typedef struct{
  double u,v;
}mapcoord_type;

// The object type
typedef struct {
  char name[25];
  char modelPath[256];
  char texturePath[256];
  double resizeMult;

  unsigned short vertices_qty;
  unsigned short polygons_qty;

  vector<vertex_type> vertex; 
  vector<polygon_type> polygon;
  vector<mapcoord_type> mapcoord;
  int id_texture;
} object3d, *object3d_ptr;



/**********************************************************
*
* FUNCTION Load3DS (object3d_ptr, char *)
*
* This function loads a mesh from a 3ds file.
* Please note that we are loading only the vertices, polygons and mapping lists.
* If you need to load meshes with advanced features as for example: 
* multi objects, materials, lights and so on, you must insert other chunk parsers.
*
*********************************************************/

extern char Load3DS (object3d_ptr ogg, char *filename);

#endif