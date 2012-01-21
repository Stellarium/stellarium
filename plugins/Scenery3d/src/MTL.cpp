#include "MTL.hpp"
#include "Util.hpp"
#include "StelFileMgr.hpp"
#include <cstdio>
#include <fstream>

using namespace std;

MTL::MTL(void)
    :loaded(false), mtlMap(), textureMapGL()
{
}

MTL::~MTL(void)
{
    mtlMap.clear();
}

void MTL::load(const char* filename)
{
    qDebug() << "MTL: loading " << filename;
    basePath = string(StelFileMgr::dirName(filename).toAscii() + "/");

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
                        { // GZ: this feeds the old material at last into the map. What happens to the last? See below.
                            //qDebug() << "MTL: newmtl " << entryStr.c_str();
                            mtlMap[entryStr] = material;
                        }
                        // GZ: allow spaces in material names:
                        //entryStr = parts[1];
                        entryStr = line.substr(line.find(parts[1]));
                        trim_right(entryStr);
                    }
                } else if (parts[0] == "Kd") {
                    if (parts.size() > 4) {
                        material.color.r = parseFloat(parts[1]);
                        material.color.g = parseFloat(parts[2]);
                        material.color.b = parseFloat(parts[3]);
                    }
                } else if (parts[0] == "Ka") {
                    if (parts.size() > 4) {
                        material.ambient.r = parseFloat(parts[1]);
                        material.ambient.g = parseFloat(parts[2]);
                        material.ambient.b = parseFloat(parts[3]);
                    }
                } else if (parts[0] == "Ks") {
                    if (parts.size() > 4) {
                        material.specular.r = parseFloat(parts[1]);
                        material.specular.g = parseFloat(parts[2]);
                        material.specular.b = parseFloat(parts[3]);
                    }
                } else if (parts[0] == "map_Kd") {
                    if (parts.size() > 1) {
                        //material.texture = parts[1]; // GZ: fails for spaces in texture names!
                        material.texture = line.substr(line.find(parts[1]));
                        //qDebug() << "MTL: From Line: " << line.c_str() << "we derive";
                        //qDebug() << "MTL: map_Kd " << material.texture.c_str();

                        trim_right(material.texture);
                        size_t position = material.texture.find("\\");
                        while (position != string::npos)
                        {
                            material.texture.replace(position, 1, "/");
                            position = material.texture.find("\\", position + 1);
                        }
                    }
                } else if (parts[0] == "map_bump" || parts[0] == "bump"){
                    if (parts.size() > 1){
                        material.bump_texture = line.substr(line.find(parts[1]));

                        trim_right(material.bump_texture);
                        size_t position = material.bump_texture.find("\\");
                        while (position != string::npos)
                        {
                            material.bump_texture.replace(position, 1, "/");
                            position = material.bump_texture.find("\\", position + 1);
                        }
                    }
                } else if (parts[0] == "Ns") {
                    if (parts.size() > 1) {
                        material.shininess = parseInt(parts[1]);
                    }
                }
            }
        }

        // GZ bugfix: We may have a material description open. Store it into the map.
        if (!entryStr.empty())
        {
            //qDebug() << "MTL: newmtl " << entryStr.c_str();
            mtlMap[entryStr] = material;
        }
    }
    else {
        qDebug() << "MTL ERROR: Couldn't load " <<  filename;
    }
}

void MTL::uploadTexturesGL(void)
{
    StelTextureMgr textureMgr;
    for (MaterialMap::iterator it = mtlMap.begin(); it != mtlMap.end(); it++) {
        string texture = it->second.texture;
        if (texture.size() > 0) {
            StelTextureSP tex = textureMgr.createTexture(QString(absolutePath(texture).c_str()), StelTexture::StelTextureParams(true, GL_LINEAR, GL_REPEAT));

            if(!tex.isNull()){
                textureMapGL[texture] = tex;
                //textureMapGL[texture] = load_texture(absolutePath(texture).c_str());
                qDebug() << "[Scenery3d] Loaded Texture: " << texture.c_str();
            }
        }

        string bump = it->second.bump_texture;
        if(bump.size() > 0)
        {
            StelTextureSP bumpTex = textureMgr.createTexture(QString(absolutePath(bump).c_str()), StelTexture::StelTextureParams(true, GL_LINEAR, GL_REPEAT));

            if(!bumpTex.isNull()){
                textureMapGL[bump] = bumpTex;
                qDebug() << "[Scenery3d] Loaded Normal Map: " << bump.c_str();
            }
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

