/*
* ---------------- www.spacesimulator.net --------------
*   ---- Space simulators and 3d engine tutorials ----
*
* Author: Damiano Vitulli <info@spacesimulator.net>
*
* ALL RIGHTS RESERVED
*
*
* Tutorial 4: 3d engine - 3ds models loader
* 
* Include File: 3dsloader.cpp
*  
*/

#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <io.h>
#include "3dsloader.h"


/**********************************************************
*
* FUNCTION Load3DS (obj_type_ptr, char *)
*
* This function loads a mesh from a 3ds file.
* Please note that we are loading only the vertices, polygons and mapping lists.
* If you need to load meshes with advanced features as for example: 
* multi objects, materials, lights and so on, you must insert other chunk parsers.
*
*********************************************************/
char Load3DS (object3d_ptr p_object, char *p_filename)
{
  int i; //Index variable

  FILE *l_file; //File pointer

  unsigned short l_chunk_id; //Chunk identifier
  unsigned int l_chunk_lenght; //Chunk lenght

  unsigned char l_char; //Char variable
  unsigned short l_qty; //Number of elements in each chunk

  unsigned short l_face_flags; //Flag that stores some face information

  if ((l_file=fopen (p_filename, "rb"))== NULL) return 0; //Open the file

  while (ftell (l_file) < filelength (fileno (l_file))) //Loop to scan the whole file 
  {
    fread (&l_chunk_id, 2, 1, l_file); //Read the chunk header
    fread (&l_chunk_lenght, 4, 1, l_file); //Read the lenght of the chunk
 
    switch (l_chunk_id)
    {
      //----------------- MAIN3DS -----------------
      // Description: Main chunk, contains all the other chunks
      // Chunk ID: 4d4d 
      // Chunk Lenght: 0 + sub chunks
      //-------------------------------------------
    case 0x4d4d: 
      break;    

      //----------------- EDIT3DS -----------------
      // Description: 3D Editor chunk, objects layout info 
      // Chunk ID: 3d3d (hex)
      // Chunk Lenght: 0 + sub chunks
      //-------------------------------------------
    case 0x3d3d:
      break;

      //--------------- EDIT_OBJECT ---------------
      // Description: Object block, info for each object
      // Chunk ID: 4000 (hex)
      // Chunk Lenght: len(object name) + sub chunks
      //-------------------------------------------
    case 0x4000: 
      i=0;
      do
      {
        fread (&l_char, 1, 1, l_file);
        p_object->name[i]=l_char;
        i++;
      }while(l_char != '\0' && i<20);
      break;

      //--------------- OBJ_TRIMESH ---------------
      // Description: Triangular mesh, contains chunks for 3d mesh info
      // Chunk ID: 4100 (hex)
      // Chunk Lenght: 0 + sub chunks
      //-------------------------------------------
    case 0x4100:
      break;

      //--------------- TRI_VERTEXL ---------------
      // Description: Vertices list
      // Chunk ID: 4110 (hex)
      // Chunk Lenght: 1 x unsigned short (number of vertices) 
      //             + 3 x float (vertex coordinates) x (number of vertices)
      //             + sub chunks
      //-------------------------------------------
    case 0x4110: 
      fread (&l_qty, sizeof (unsigned short), 1, l_file);
      p_object->vertices_qty = l_qty;
      for (i=0; i<l_qty; i++)
      {
        vertex_type dummyVertex;
        float dummy;
        fread (&dummy, sizeof(float), 1, l_file);
        dummyVertex.x = (double)dummy*p_object->resizeMult;
        fread (&dummy, sizeof(float), 1, l_file);
        dummyVertex.y = (double)dummy*p_object->resizeMult;
        fread (&dummy, sizeof(float), 1, l_file);
        dummyVertex.z = (double)dummy*p_object->resizeMult;

        p_object->vertex.push_back(dummyVertex);
      }
      break;

      //--------------- TRI_FACEL1 ----------------
      // Description: Polygons (faces) list
      // Chunk ID: 4120 (hex)
      // Chunk Lenght: 1 x unsigned short (number of polygons) 
      //             + 3 x unsigned short (polygon points) x (number of polygons)
      //             + sub chunks
      //-------------------------------------------
    case 0x4120:
      fread (&l_qty, sizeof (unsigned short), 1, l_file);
      p_object->polygons_qty = l_qty;
      for (i=0; i<l_qty; i++)
      {
        polygon_type dummyPoligon;
        fread (&dummyPoligon.a, sizeof(unsigned short), 1, l_file);
        fread (&dummyPoligon.b, sizeof(unsigned short), 1, l_file);
        fread (&dummyPoligon.c, sizeof(unsigned short), 1, l_file);
        p_object->polygon.push_back(dummyPoligon);
        fread (&l_face_flags, sizeof (unsigned short), 1, l_file);
      }
      break;

      //------------- TRI_MAPPINGCOORS ------------
      // Description: Vertices list
      // Chunk ID: 4140 (hex)
      // Chunk Lenght: 1 x unsigned short (number of mapping points) 
      //             + 2 x float (mapping coordinates) x (number of mapping points)
      //             + sub chunks
      //-------------------------------------------
    case 0x4140:
      fread (&l_qty, sizeof (unsigned short), 1, l_file);
      for (i=0; i<l_qty; i++)
      {
        mapcoord_type dummyMap;
        float dummy;
        fread (&dummy, sizeof (float), 1, l_file);
        dummyMap.u = (double)dummy;
        fread (&dummy, sizeof (float), 1, l_file);
        dummyMap.v = (double)dummy;
        p_object->mapcoord.push_back(dummyMap);
      }
      break;

      //----------- Skip unknow chunks ------------
      //We need to skip all the chunks that currently we don't use
      //We use the chunk lenght information to set the file pointer
      //to the same level next chunk
      //-------------------------------------------
    default:
      fseek(l_file, l_chunk_lenght-6, SEEK_CUR);
    } 
  }
  fclose (l_file); // Closes the file stream
  return (1); // Returns ok
}
