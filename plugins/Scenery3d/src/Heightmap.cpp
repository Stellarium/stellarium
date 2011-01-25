#include <limits>

#include "Heightmap.hpp"
#include "VecMath.hpp"
#include <cassert>

#define INF (std::numeric_limits<float>::max())
#define NO_HEIGHT (-INF)

#define HMDEBUG(S) qDebug() << S;

Heightmap::Heightmap(const OBJ& obj) : obj(obj)
{
   this->xMin = INF;
   this->xMax = -INF;
   this->yMin = INF;
   this->yMax = -INF;
   
   for (std::vector<OBJ::Vertex>::const_iterator it = obj.vertices.begin(); it != obj.vertices.end(); it++)
   {
      if (it->x < xMin) xMin = it->x;
      if (it->y < yMin) yMin = it->y;
      if (it->x > xMax) xMax = it->x;
      if (it->y > yMax) yMax = it->y;
   }
   
   this->initGrid();
}

Heightmap::~Heightmap()
{
   delete[] grid;
}

/**
 * Returns the height of the ground model for any observer x/y coords.
 * The height is the highest z value of the ground model at these
 * coordinates.
 */
float Heightmap::getHeight(float x, float y)
{
   Heightmap::GridSpace* space = getSpace(x, y);
   if (space == NULL) 
   {
      return 0;
   }
   else
   {
      float h = space->getHeight(obj, x, y);
      if (h == NO_HEIGHT)
      {
			return 0;
      }
      else
		{
			return h;
      }
   }
}

/**
 * Height query within a single grid space. The list of faces to check
 * for intersection with the observer coords is limited to faces
 * intersecting this grid space.
 */
float Heightmap::GridSpace::getHeight(const OBJ& obj, float x, float y)
{
	float h = NO_HEIGHT;

	for (size_t i = 0; i < faces.size(); i++)
	{
		const OBJ::Face* face = faces[i];
		float face_h = face_height_at(obj, face, x, y);
		if (face_h > h)
		{
			h = face_h;
		}
	}

   return h;
}

/**
 * Allocates memory for the grid and fills every grid space with
 * pointers to the faces in that space.
 */
void Heightmap::initGrid()
{
   grid = new GridSpace[GRID_LENGTH*GRID_LENGTH];
   
   for (int y = 0; y < GRID_LENGTH; y++)
   for (int x = 0; x < GRID_LENGTH; x++)
   {
		float xmin = this->xMin + (x * (this->xMax - this->xMin)) / GRID_LENGTH;
		float ymin = this->yMin + (y * (this->yMax - this->yMin)) / GRID_LENGTH;
		float xmax = this->xMin + ((x+1) * (this->xMax - this->xMin)) / GRID_LENGTH;
		float ymax = this->yMin + ((y+1) * (this->yMax - this->yMin)) / GRID_LENGTH;
      
      FaceVector* faces = &grid[y*GRID_LENGTH + x].faces;
      
		for (OBJ::ModelList::const_iterator m = obj.models.begin(); m != obj.models.end(); m++)
		{
			for (OBJ::FaceList::const_iterator face = m->faces.begin(); face != m->faces.end(); face++)
			{
				if (face_in_area (&*face, xmin, ymin, xmax, ymax))
				{
					faces->push_back(&*face);
				}
			}
		}
   }
}

/**
 * Returns the GridSpace which covers the area around x/y.
 */
Heightmap::GridSpace* Heightmap::getSpace(float x, float y)
{
   int ix = (x - xMin) / (xMax - xMin) * GRID_LENGTH;
   int iy = (y - yMin) / (yMax - yMin) * GRID_LENGTH;
   
   if ((ix < 0) || (ix >= GRID_LENGTH) || (iy < 0) || (iy >= GRID_LENGTH))
   {
		 HMDEBUG("OUT OF GRID (" << x << "," << y << ") (" << ix << "," << iy << ")");
      return NULL;
   }
   else
   {
      return &grid[iy*GRID_LENGTH + ix];
   }
}

/**
 * Returns the height of the face at the given point or -inf if
 * the coordinates are outside the bounds of the face.
 */
float Heightmap::GridSpace::face_height_at(const OBJ& obj, const OBJ::Face* face, float x, float y)
{
	Q_ASSERT(face->refs.size() == 3); // There are only triangles in our models, nothing else

	// Get vertices in triangle
	struct { float x,y,z; } p[3];
	int i = 0;
	for (OBJ::RefList::const_iterator it = face->refs.begin(); it != face->refs.end(); it++)
	{
		const OBJ::Vertex& vert = obj.vertices[it->v];
		p[i].x = vert.x;
		p[i].y = vert.y;
		p[i].z = vert.z;
		i++;
	}

	/*
	{ // HMDEBUG
		p[0].x = -30; p[0].y = -30; p[0].z = 2;
		p[1].x = 30; p[1].y = -30; p[1].z = 2;
		p[2].x = 0.; p[2].y = 30; p[2].z = 7;
	} */

	// Weight of those vertices is used to calculate exact height at (x,y), using barycentric coordinates, see also
	// http://en.wikipedia.org/wiki/Barycentric_coordinate_system_(mathematics)#Converting_to_barycentric_coordinates
	float det_T = (p[1].y - p[2].y) * (p[0].x - p[2].x) + (p[2].x - p[1].x) * (p[0].y - p[2].y);
	float l1 = ( (p[1].y - p[2].y) * (x - p[2].x) + (p[2].x - p[1].x) * (y - p[2].y) ) / det_T;
	float l2 = ( (p[2].y - p[0].y) * (x - p[2].x) + (p[0].x - p[2].x) * (y - p[2].y) ) / det_T;
	float l3 = 1. - l1 - l2;

	if ((l1 < 0) || (l2 < 0) || (l3 < 0))
	{
		return NO_HEIGHT; // (x,y) out of face bounds
	}
	else
	{
		//float h = l1 * p[0].z + l2 * p[1].z + l3 * p[2].z;
		//HMDEBUG("x,y=" << x << "," << y << "; det_T=" << det_T << ", l1=" << l1 << ", l2=" << l2 << ", l3=" << l3 << ", h=" << h);
		return l1 * p[0].z + l2 * p[1].z + l3 * p[2].z;
	}

	// temporary implementation: use z coord of first vertex found in face
	// return obj.vertices[face->refs.front().v].z;
}

/**
 * Returns true if the given face intersects the given area.
 */
bool Heightmap::face_in_area (const OBJ::Face* face, float xmin, float ymin, float xmax, float ymax)
{
   // current implementation: use face's bounding box
   float f_xmin = xmax;
   float f_ymin = ymax;
   float f_xmax = xmin;
   float f_ymax = ymin;
   
   for (OBJ::RefList::const_iterator r = face->refs.begin(); r != face->refs.end(); r++)
   {
      OBJ::Vertex vertex = obj.vertices[r->v];
      if (vertex.x < f_xmin) f_xmin = vertex.x;
      if (vertex.y < f_ymin) f_ymin = vertex.y;
      if (vertex.x > f_xmax) f_xmax = vertex.x;
      if (vertex.y > f_ymax) f_ymax = vertex.y;
   }
   
   if ((f_xmin < xmax) && (f_ymin < ymax) && (f_xmax > xmin) && (f_ymax > ymin))
	{
      return true;
   }
   else
	{
      return false;
   }
}
