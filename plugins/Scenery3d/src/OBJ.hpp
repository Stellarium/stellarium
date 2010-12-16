#ifndef _OBJ_HPP_
#define _OBJ_HPP_

#include <vector>
#include <list>
#include <string>
#include "MTL.hpp"
#include "StelCore.hpp"

class OBJ
{
    public:
        OBJ( void );
        virtual ~OBJ( void );

        void load(const char* filename);
        void drawTriGL(void); // simple triangle renderer
        void makeStelArrays(Vec3d*& vertices, Vec3f*& texcoords, Vec3f*& normals);

        struct Vertex {
            float x, y, z;
        };

        struct Texcoord {
            float u, v;
        };

        struct Ref {
            unsigned int v, t, n; // vertex, texture, normal index
            bool texture; // use texture?
            bool normal; // use normal?
        };

        typedef std::list<Ref> RefList;
        struct Face {
            RefList refs;
        };

        typedef std::list<Face> FaceList;
        struct Model {
            FaceList faces;
            std::string name;
            std::string material;
        };

    private:
        bool loaded;
        std::string basePath;

        std::vector<Vertex> vertices;
        std::vector<Texcoord> texcoords;
        std::vector<Vertex> normals;
        typedef std::list<Model> ModelList;
        ModelList models;
        MTL mtlLib;
};

#endif

