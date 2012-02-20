#ifndef _OBJ_HPP_
#define _OBJ_HPP_

#include <vector>
#include <list>
#include <string>
#include "MTL.hpp"
#include "StelCore.hpp"

using std::vector;

class Heightmap;

//! Class for loading and representing a Wavefront .obj file.
//! For details about the file format, see http://en.wikipedia.org/wiki/Wavefront_.obj_file
class OBJ
{
    public:
        //! Initialize an empty object.
        //! Usually you then call the .load() method to load the mesh from an .obj file.
        OBJ( void );
        virtual ~OBJ( void );

        //! OBJ files can have vertices encoded in different order.
        //! Only XYZ and XZY may occur in real life, but we can cope with all...
        enum vertexOrder { XYZ, XZY, YXZ, YZX, ZXY, ZYX };

        //! Structure for holding vertex arrays and OpenGL materials from the MTL for use with Stellarium.
        //! Usually those are generated and returned by calling the .getStelArrays() method.
        //! GZ TODO: Avoid copying MTL Material items into this, just take the Material directly!
        struct StelModel {
            StelModel() : texture(), bump_texture(), diffuseColor(0.8f, 0.8f, 0.8f, 1.0f),
                          ambientColor(0.2f, 0.2f, 0.2f, 1.0f), specularColor(0.0f, 0.0f, 0.0f, 1.0f), shininess(0.0f),
                          vertices(NULL), normals(NULL), texcoords(NULL) {}
            long triangleCount; //!< Total number of triangles in the model.
            StelTextureSP texture;      //!< Shared pointer to texture of the model. This can be null.
            StelTextureSP bump_texture; //!< Shared pointer to bump map texture of the model. This can be null.
            Vec4f diffuseColor;  //!< Directly lit color of the model. The default value is (0.8, 0.8, 0.8, 1.0) and should be set to the MTL Kd
            Vec4f ambientColor;  //!< "Background" color of the model. The default value is (0.2, 0.2, 0.2, 1.0) and should be set to the MTL Ka
            Vec4f specularColor; //!< Highlight color of the model. The default value is (0.0, 0.0, 0.0, 1.0) and should be set to the MTL Ks
            float shininess;  //!< Specular exponent of the model. The default value is 0 and should be set to the MTL Ns [0..128]
            MTL::Illum illum; //!< illumination model, copied from Material.
            Vec3d* vertices;  //!< Vertices array. Length is triangleCount * 3.
            Vec3f* normals;   //!< Normals array. Length is triangleCount * 3.
            Vec2f* texcoords; //!< Texcoords array. Length is triangleCount * 3.
            //Vec3f* tangents; //!< Tanget array. Length is triangleCount * 3.
        };

        //! Load mesh data from an .obj file.
        //! @param filename File name of the .obj file.
        void load(const char* filename, const enum vertexOrder order=XYZ);
        // //! Draw triangles using OpenGL. This method is not in use with Stellarium.
        // void drawTriGL(void); // simple triangle renderer
        //! Generate and return a list of StelModel structures for rendering vertex arrays with Stellarium.
        vector<StelModel> getStelArrays();
        //! Transform all the vertices through multiplication with a 4x4 matrix.
        //! @param mat Matrix to multiply vertices with.
        void transform(Mat4d mat);


        //! Holds a vertex or normal entry from the .obj file.
        struct Vertex {
            float x, y, z; //!< coordinates
        };

        //! Holds a texcoord entry from the .obj file.
        struct Texcoord {
            float u, v; //!< coordinates
        };

        //! Holds a vertex reference, that is, a combination of vertex, texcoord, normal indices from the .obj file.
        struct Ref {
            unsigned int v, t, n; // vertex, texture, normal index. The last 2 may not be here, therefore:
            bool texcoordValid;
            bool normalValid;
        };

        typedef std::list<Ref> RefList;
        //! Holds a face entry from the .obj file.
        struct Face {
            RefList refs; //!< List of vertex/texcoord/normal references; length is number of vertices that compose the face.
        };

        typedef std::list<Face> FaceList;
        //! Holds a model from the .obj file.
        struct Model {
            FaceList faces; //!< List of polygon faces.
            std::string name; //!< Name of the model, as given in the .obj file.
            std::string material; //!< Name of the model's material, as given in the .obj file.
        };

        //! @return Minimum X coordinate of all vertices.
        float getMinX() { return minX; }
        //! @return Minimum Y coordinate of all vertices.
        float getMinY() { return minY; }
        //! @return Minimum Z coordinate of all vertices.
        float getMinZ() { return minZ; }
        //! @return Maximum X coordinate of all vertices.
        float getMaxX() { return maxX; }
        //! @return Maximum Y coordinate of all vertices.
        float getMaxY() { return maxY; }
        //! @return Maximum Z coordinate of all vertices.
        float getMaxZ() { return maxZ; }

    private:
        bool loaded;
        std::string basePath;

        std::vector<Vertex> vertices;
        std::vector<Texcoord> texcoords;
        std::vector<Vertex> normals;
        //std::vector<Vec3f> tangents;
        typedef std::list<Model> ModelList;
        ModelList models;
        MTL mtlLib;

        float minX, maxX, minY, maxY, minZ, maxZ;

        friend class Heightmap;

        //void generateTangents(Model model);
};

#endif

