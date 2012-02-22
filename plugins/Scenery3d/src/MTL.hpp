#ifndef _MTL_HPP_
#define _MTL_HPP_

#include <string>
#include <map>
#include "VecMath.hpp"
#include "StelTextureMgr.hpp"


//! Class for loading and representing a .mtl file.
//! MTL files are used with Wavefront .obj files for
//! storing material information, such as textures and colors.
//! For details about the file format, see http://en.wikipedia.org/wiki/Wavefront_.obj_file#Material_template_library
class MTL
{
    public:
        typedef Vec4f Color;
        enum Illum { DIFFUSE, DIFFUSE_AND_AMBIENT, SPECULAR }; //!< Supported OpenGL illumination models. Use specular sparingly!

        //! Struct for holding a material definition.
        struct Material { // The initial values are the OpenGL defaults.
            Material() : diffuse(0.8f, 0.8f, 0.8f, 1.0f), ambient(0.2f, 0.2f, 0.2f, 1.0f), specular(0.0f,0.0f,0.0f, 1.0f),
                         shininess(0.0f), opacity(1.0f), illum(DIFFUSE), texture(), bump_texture() {}
            Color diffuse;   //!< Material diffuse color.
            Color ambient;   //!< Ambient color.
            Color specular;  //!< Specular color.
            float shininess; //!< Material shininess factor. (0..128)
            float opacity;   //!< Material opacity (0..1)
            Illum illum;     //!< illumination model to be used with this material. See http://en.wikipedia.org/wiki/Wavefront_.obj_file, "illum"
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

