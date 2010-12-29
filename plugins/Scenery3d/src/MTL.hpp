#ifndef _MTL_HPP_
#define _MTL_HPP_

#include <string>
#include <map>
#include "StelTextureMgr.hpp"

class MTL
{
    public:
        struct Color {
            Color() : r(1.0f), g(1.0f), b(1.0f) {}
            float r, g, b;
        };

        struct Material {
            Material() : color(), texture(), shininess(0) {}
            Color color;
            std::string texture;
            int shininess;
        };

        MTL(void);
        virtual ~MTL(void);

        void load(const char* filename);
        void uploadTexturesGL(void);
        const Material& getMaterial(std::string matName);
        StelTextureSP getTexture(std::string texName);
    private:
        std::string absolutePath(std::string path);

        bool loaded;
        std::string basePath;

        typedef std::map<std::string, Material> MaterialMap;
        MaterialMap mtlMap;
        std::map<std::string, StelTextureSP> textureMapGL;
};

#endif

