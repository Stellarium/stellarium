#include "MTL.hpp"
#include "Util.hpp"
#include "PathInfo.hpp"
#include <cstdio>
#include <fstream>

using namespace std;

MTL::MTL(void)
    :loaded(false), mtlMap(), textureMapGL()
{
}

MTL::~MTL(void)
{
}

void MTL::load(const char* filename)
{
    basePath = string(PathInfo::getDirectory(filename));
    ifstream file(filename);
    string line;
    if (file.is_open()) {
        Material material;
        string entryStr = "";
        while (!file.eof()) {
            getline(file, line);
            vector<string> parts = splitStr(line, ' ');
            if (parts.size() > 0) {
                if (parts[0] == "newmtl") {
                    if (parts.size() > 1) {
                        if (!entryStr.empty())
                        {
                            printf("newmtl %s.\n", entryStr.c_str());
                            mtlMap[entryStr] = material;
                        }
                        entryStr = parts[1];
                        trim_right(entryStr);
                    }
                } else if (parts[0] == "Kd") {
                    if (parts.size() > 4) {
                        material.color.r = parseFloat(parts[1]);
                        material.color.g = parseFloat(parts[2]);
                        material.color.b = parseFloat(parts[3]);
                    }
                } else if (parts[0] == "map_Kd") {
                    if (parts.size() > 1) {
                        material.texture = parts[1];
                        trim_right(material.texture);
                        size_t position = material.texture.find("\\");
                        while (position != string::npos)
                        {
                            material.texture.replace(position, 1, "/");
                            position = material.texture.find("\\", position + 1);
                        }
                    }
                } else if (parts[0] == "Ns") {
                    if (parts.size() > 1) {
                        material.shininess = parseInt(parts[1]);
                    }
                }
            }
        }
    } else {
        fprintf(stderr, "ERROR: Couldn't load %s.\n", filename);
    }
}

void MTL::uploadTexturesGL(void)
{
    StelTextureMgr textureMgr;
    for (MaterialMap::iterator it = mtlMap.begin(); it != mtlMap.end(); it++) {
        string texture = it->second.texture;
        if (texture.size() > 0) {
            textureMapGL[texture] = textureMgr.createTexture(QString(absolutePath(texture).c_str()), StelTexture::StelTextureParams(true, GL_LINEAR, GL_REPEAT));
            //textureMapGL[texture] = load_texture(absolutePath(texture).c_str());
        }
    }
}

string MTL::absolutePath(string path)
{
    return basePath + path;
}

const MTL::Material& MTL::getMaterial(string matName)
{
    return mtlMap[matName];
}

StelTextureSP MTL::getTexture(string texName)
{
    return textureMapGL[texName];
}

