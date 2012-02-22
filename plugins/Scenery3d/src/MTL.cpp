#include "MTL.hpp"
#include "Util.hpp"
#include "StelFileMgr.hpp"
#include <cstdio>
#include <fstream>

MTL::MTL(void)
    :loaded(false), mMap(), textureMapGL()
{
}

MTL::~MTL(void)
{
    mMap.clear();
}

void MTL::load(const char *filename)
{
    qDebug() << "[Scenery3d] MTL loading: " << filename;
    bool matOpen = false;
    Material *curMaterial = new Material();
    std::string curLine;
    basePath = std::string(StelFileMgr::dirName(filename).toAscii() + "/");
    std::ifstream file(filename);

    if(file.is_open())
    {
        std::string entryStr = "";
        while(!file.eof())
        {
            std::getline(file, curLine);
            std::vector<std::string> parts = splitStr(curLine, ' ');
            if(parts.size() > 0)
            {
                if(parts[0] == "newmtl")
                {
                    //Open material
                    matOpen = true;
                    //Create new material
                    curMaterial = new Material();
                    //Find material's name
                    entryStr = curLine.substr(curLine.find(parts[1]));
                    trim_right(entryStr);
                    //Store it
                    mMap[entryStr] = curMaterial;
                }
                else if(parts[0] == "Kd" && matOpen)
                {
                    if (parts.size() > 3)
                    {
                        curMaterial->diffuse.v[0] = parseFloat(parts[1]);
                        curMaterial->diffuse.v[1] = parseFloat(parts[2]);
                        curMaterial->diffuse.v[2] = parseFloat(parts[3]);
                    }
                }
                else if(parts[0] == "Ka" && matOpen)
                {
                    if(parts.size() > 3)
                    {
                        curMaterial->ambient.v[0] = parseFloat(parts[1]);
                        curMaterial->ambient.v[1] = parseFloat(parts[2]);
                        curMaterial->ambient.v[2] = parseFloat(parts[3]);
                    }
                }
                else if(parts[0] == "Ks" && matOpen)
                {
                    if(parts.size() > 3)
                    {
                        curMaterial->specular.v[0] = parseFloat(parts[1]);
                        curMaterial->specular.v[1] = parseFloat(parts[2]);
                        curMaterial->specular.v[2] = parseFloat(parts[3]);
                    }
                }
                else if (parts[0] == "map_Kd" && matOpen)
                {
                    if (parts.size() > 1)
                    {
                        //material.texture = parts[1]; // GZ: fails for spaces in texture names!
                        curMaterial->texture = curLine.substr(curLine.find(parts[1]));
                        //qDebug() << "MTL: From Line: " << line.c_str() << "we derive";
                        //qDebug() << "MTL: map_Kd " << material.texture.c_str();

                        trim_right(curMaterial->texture);
                        size_t position = curMaterial->texture.find("\\");
                        while (position != std::string::npos)
                        {
                            curMaterial->texture.replace(position, 1, "/");
                            position = curMaterial->texture.find("\\", position + 1);
                        }
                    }
                }
                else if ((parts[0] == "map_bump" || parts[0] == "bump") && matOpen)
                {
                    if (parts.size() > 1)
                    {
                        curMaterial->bump_texture = curLine.substr(curLine.find(parts[1]));

                        trim_right(curMaterial->bump_texture);
                        size_t position = curMaterial->bump_texture.find("\\");
                        while (position != std::string::npos)
                        {
                            curMaterial->bump_texture.replace(position, 1, "/");
                            position = curMaterial->bump_texture.find("\\", position + 1);
                        }
                    }
                }
                else if (parts[0] == "Ns" && matOpen)
                {
                    if (parts.size() > 1)
                    {
                        curMaterial->shininess = parseFloat(parts[1]);
                    }
                }
                else if (parts[0] == "d" && matOpen)
                {
                    if (parts.size() > 1)
                    {
                        curMaterial->opacity = parseFloat(parts[1]);
                    }
                }
                else if (parts[0] == "illum" && matOpen)
                {
                    if (parts.size() > 1)
                    {
                        curMaterial->illum = (MTL::Illum)  qMin((unsigned int)2, parseInt(parts[1]));
                    }
                }
            }
        }
    }
    else
    {
        qWarning() << "[Scenery3d] MTL ERROR: Couldn't load " <<  filename;
    }
}

void MTL::uploadTexturesGL(void)
{
    StelTextureMgr textureMgr;
    for (MatMap::iterator it = mMap.begin(); it != mMap.end(); it++) {
        std::string texture = it->second->texture;
        qDebug() << "[Scenery3d]: TEX: " << texture.c_str();
        if (texture.size() > 0) {
            StelTextureSP tex = textureMgr.createTexture(QString(absolutePath(texture).c_str()), StelTexture::StelTextureParams(true, GL_LINEAR, GL_REPEAT));

            if(!tex.isNull()){
                textureMapGL[texture] = tex;
                //qDebug() << "[Scenery3d] Loaded Texture: " << texture.c_str();
            }
            else
                qWarning() << "[Scenery3d] Failed to load Texture: " << texture.c_str();
        }

        std::string bump = it->second->bump_texture;
        qWarning() << "[Scenery3d]: BUMP: " << bump.c_str();
        if(bump.size() > 0)
        {
            StelTextureSP bumpTex = textureMgr.createTexture(QString(absolutePath(bump).c_str()), StelTexture::StelTextureParams(true, GL_LINEAR, GL_REPEAT));

            if(!bumpTex.isNull()){
                textureMapGL[bump] = bumpTex;
                //qDebug() << "[Scenery3d] Loaded Normal Map: " << bump.c_str();
            }
            else
                qWarning() << "[Scenery3d] Failed to load Normal Map: " << bump.c_str();
        }

        qDebug() << "[Scenery3d]: Kd: r" << it->second->diffuse.v[0] << " g" << it->second->diffuse.v[1] << " b" << it->second->diffuse.v[2];
    }
}

std::string MTL::absolutePath(std::string path)
{
    return basePath + path;
}

const MTL::Material* MTL::getMaterial(std::string matName)
{
    return mMap[matName];
}

StelTextureSP MTL::getTexture(std::string texName)
{
    return textureMapGL[texName];
}

