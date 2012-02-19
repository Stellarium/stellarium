#ifndef _MTL_HPP_
#define _MTL_HPP_

#include <string>
#include <map>
#include "StelTextureMgr.hpp"


//! Class for loading and representing a .mtl file.
//! MTL files are used with Wavefront .obj files for
//! storing material information, such as textures and colors.
//! For details about the file format, see http://en.wikipedia.org/wiki/Wavefront_.obj_file#Material_template_library
class MTL
{
    public:
        //! Struct for holding a single RGB color.
        struct Color {
            Color() : r(1.0f), g(1.0f), b(1.0f) {}
            Color(float nr, float ng, float nb):  r(nr), g(ng), b(nb){}
            float r, g, b;
        };

        //! Struct for holding a material definition.
        struct Material {
            Material() : diffuse(), ambient(0.f,0.f,0.f), specular(0.f,0.f,0.f), shininess(0), texture(), bump_texture() {}
            Color diffuse;  //!< Material diffuse color.
            Color ambient;  //!< Ambient color.
            Color specular; //!< Specular color.
            int shininess;  //!< Material shininess factor.
            std::string texture; //!< Filename of the texture file.
            std::string bump_texture; //!< Filename of the bump map texture file.
        };

        //! Initialize an empty MTL object.
        //! Usually, you then load a file using the .load() method.
        MTL(void);
        virtual ~MTL(void);

        //! Load an .mtl file into memory.
        void load(const char* filename);
        //! Upload all textures to GPU memory using OpenGL.
        void uploadTexturesGL(void);
        //! Returns a stored material definition.
        //! @param matName Name of the material, as defined in the .mtl file.
        //! @return Reference to the material definition.
        const Material* getMaterial(std::string matName);
        //! Returns a shared pointer to a StelTexture reference.
        //! Make sure to call uploadTexturesGL() before using this.
        //! @param texName File name of the texture as defined in the .mtl file.
        StelTextureSP getTexture(std::string texName);
        //! @return number of materials in map.
        int size() { return mMap.size(); }
    private:
        std::string absolutePath(std::string path);

        bool loaded;
        std::string basePath;

        typedef std::map<std::string, Material*> MatMap;
        MatMap mMap;
        //typedef std::map<std::string, Material> MaterialMap;
        //MaterialMap mtlMap;
        std::map<std::string, StelTextureSP> textureMapGL; 
};

#endif

