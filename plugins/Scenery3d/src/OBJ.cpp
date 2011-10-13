#include "OBJ.hpp"

#include <fstream>
#include <sstream>
#include <iostream>
#include <limits>

#include "Util.hpp"
#include "StelFileMgr.hpp"

using namespace std;

OBJ::OBJ( void )
    :loaded(false), vertices(), texcoords(), normals(), models()
{
    minX = maxX = 0.0f;
    minY = maxY = 0.0f;
    minZ = maxZ = 0.0f;
}

OBJ::~OBJ( void )
{
}

void OBJ::load( const char* filename, const enum vertexOrder order )
{
    basePath = string(StelFileMgr::dirName(filename).toAscii() + "/");

    long totalFaces=0;
    ifstream file(filename);
    string line;
    minX = minY = minZ =  numeric_limits<float>::max();
    maxX = maxY = maxZ = -numeric_limits<float>::max();
    if (file.is_open()) {
        Model model;
        while (!file.eof()) {
            getline(file, line);
            vector<string> parts = splitStr(line, ' ');
            if (parts.size() > 0) {
                if (parts[0] == "v") { // vertex (x,y,z)
                    if (parts.size() >= 4) {
                        Vertex v;
                        switch (order) {
                        case XYZ:
                            v.x = parseFloat(parts[1]);
                            v.y = parseFloat(parts[2]);
                            v.z = parseFloat(parts[3]);
                            break;
                        case XZY:
                            v.x = parseFloat(parts[1]);
                            v.z = parseFloat(parts[2]);
                            v.y = parseFloat(parts[3]);
                            break;
                        case YXZ:
                            v.y = parseFloat(parts[1]);
                            v.x = parseFloat(parts[2]);
                            v.z = parseFloat(parts[3]);
                            break;
                        case YZX:
                            v.y = parseFloat(parts[1]);
                            v.z = parseFloat(parts[2]);
                            v.x = parseFloat(parts[3]);
                            break;
                        case ZXY:
                            v.z = parseFloat(parts[1]);
                            v.x = parseFloat(parts[2]);
                            v.y = parseFloat(parts[3]);
                            break;
                        case ZYX:
                            v.z = parseFloat(parts[1]);
                            v.y = parseFloat(parts[2]);
                            v.x = parseFloat(parts[3]);
                            break;
                        default:
                            qDebug() << "Scenery3d plugin - Invalid vertex order, Programming bug!";
                        }
                        minX = min(v.x, minX);
                        minY = min(v.y, minY);
                        minZ = min(v.z, minZ);
                        maxX = max(v.x, maxX);
                        maxY = max(v.y, maxY);
                        maxZ = max(v.z, maxZ);
                        vertices.push_back(v);
                    }
                } else if (parts[0] == "vt") { // texture (u,v)
                    if (parts.size() >= 3) {
                        Texcoord vt;
                        vt.u = parseFloat(parts[1]);
                        vt.v = parseFloat(parts[2]);
                        texcoords.push_back(vt);
                    }
                } else if (parts[0] == "vn") { // normal (x,y,z)
                    if (parts.size() >= 4) {
                        Vertex vn;
                        switch (order) {
                        case XYZ:
                            vn.x = parseFloat(parts[1]);
                            vn.y = parseFloat(parts[2]);
                            vn.z = parseFloat(parts[3]);
                            break;
                        case XZY:
                            vn.x = parseFloat(parts[1]);
                            vn.z = parseFloat(parts[2]);
                            vn.y = parseFloat(parts[3]);
                            break;
                        case YXZ:
                            vn.y = parseFloat(parts[1]);
                            vn.x = parseFloat(parts[2]);
                            vn.z = parseFloat(parts[3]);
                            break;
                        case YZX:
                            vn.y = parseFloat(parts[1]);
                            vn.z = parseFloat(parts[2]);
                            vn.x = parseFloat(parts[3]);
                            break;
                        case ZXY:
                            vn.z = parseFloat(parts[1]);
                            vn.x = parseFloat(parts[2]);
                            vn.y = parseFloat(parts[3]);
                            break;
                        case ZYX:
                            vn.z = parseFloat(parts[1]);
                            vn.y = parseFloat(parts[2]);
                            vn.x = parseFloat(parts[3]);
                            break;
                        default:
                            qDebug() << "Scenery3d plugin - Invalid vertex order at v normals, Programming bug!";
                    }
                        normals.push_back(vn);
                    }
                } else if (parts[0] == "g") { // polygon group
                    if (parts.size() > 1) {
                        if (model.faces.size() > 0) {
                            models.push_back(model);
                            model = Model();
			    model.name = parts[1];
			}
                    }
                } else if (parts[0] == "usemtl") { // use material
                    if (parts.size() > 1) {
                        //model.material = parts[1];// GZ: fails for spaces in material names!
                        model.material = line.substr(line.find(parts[1]));
                        trim_right(model.material);
                        //printf("usemtl %s.\n", model.material.c_str());
                        qDebug() << "OBJ: usemtl " << model.material.c_str();
                    }
                } else if (parts[0] == "f") { // face
                    Face face;
                    for (unsigned int i=1; i<parts.size(); ++i) {
                        Ref ref={0, 0, 0, 0, 0};
                        vector<string> f = splitStr(parts[i], '/');
                        if (f.size() >= 2) { // ONLY for vertices with either texture or texture and normals
                            ref.v = parseInt(f[0]) - 1;
                            ref.texture = false;
                            if (f[1].size() > 0) { // if string is valid
                                ref.t = parseInt(f[1]) - 1;
                                ref.texture = true;
                            }
                            ref.normal = false;
                            if (f.size() > 2) {
                                ref.n = parseInt(f[2]) - 1;
                                ref.normal = true;
                            }
                        }
                        face.refs.push_back(ref);
                    }
                    if (face.refs.size() >= 3) {
                        model.faces.push_back(face);
                        totalFaces++;
                    }
                } else if (parts[0] == "mtllib") {
                    if (parts.size() > 1) {
                        //string path = basePath + parts[1];// GZ: fails for spaces in file names!
                        string path = basePath + line.substr(line.find(parts[1]));
                        trim_right(path);
                        qDebug() << "OBJ: mtllib " << path.c_str();
                        mtlLib.load(path.c_str());
                        mtlLib.uploadTexturesGL();
                    }
		} else if (line[0]=='#') {
		    // skip comment line
		} else {
		    qDebug() << "Unknown .OBJ feature: " << parts[0].c_str();
		}
	    }
        }
        if (model.faces.size() > 0) {
            models.push_back(model);
            model = Model();
        }
        loaded = true;
	qDebug() << vertices.size() << " vertices,";
	qDebug() << normals.size() << " normals,";
	qDebug() << totalFaces << " faces,";
	qDebug() << texcoords.size() << " texture coordinates,";
	qDebug() << models.size() << " models (OBJ groups),";
	qDebug() << mtlLib.size() << " materials.";
	qDebug() << filename << " loaded.";
	qDebug() << "X: [" << minX << ", " << maxX << "] ";
	qDebug() << "Y: [" << minY << ", " << maxY << "] ";
	qDebug() << "Z: [" << minZ << ", " << maxZ << "]";
    } else {
        qDebug() << "Couldn't open " << filename;
    }
}

void OBJ::drawTriGL( void ) // simple triangle renderer
{
    /*
    for (ModelList::iterator it1 = models.begin(); it1 != models.end(); it1++) {
        Model& model = *it1;
        const MTL::Material& material = mtlLib.getMaterial(model.material);
        if (!material.texture.empty())
        {
            glBindTexture(GL_TEXTURE_2D, mtlLib.getTexture(material.texture));
        }
        else
        {
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        glColor3f(material.color.r, material.color.g, material.color.b);
        glBegin(GL_TRIANGLES);
        for (FaceList::iterator it2 = model.faces.begin(); it2 != model.faces.end(); it2++) {
            Face& face = *it2;
            int three = 0;
            for (RefList::iterator it3 = face.refs.begin(); it3 != face.refs.end(); it3++) {
                Ref& ref = *it3;
                Vertex& vertex = vertices[ref.v];
                if (ref.texture)
                {
                    Texcoord& texcoord = texcoords[ref.t];
                    glTexCoord2f(texcoord.u, texcoord.v);
                }
                if (ref.normal)
                {
                    Vertex& normal = normals[ref.n];
                    glNormal3f(normal.x, normal.y, normal.z);
                }
                glVertex3f(vertex.x, vertex.y, vertex.z);
                three++;
            }
            if (three != 3) {
                fprintf(stderr, "%d-polygon is not a triangle\n", three);
            }
        }
        glEnd();
    }
    */
}

vector<OBJ::StelModel> OBJ::getStelArrays()
{
    vector<StelModel> stelModels;
    StelModel stelModel;
    for (ModelList::iterator it1 = models.begin(); it1 != models.end(); it1++) {
        Model& model = *it1;
        const MTL::Material& material = mtlLib.getMaterial(model.material);
        if (!material.texture.empty()) {
            stelModel.texture = mtlLib.getTexture(material.texture);
        } else {
            stelModel.texture.clear();
        }
        stelModel.triangleCount = model.faces.size();
        stelModel.color = Vec3f(material.color.r, material.color.g, material.color.b);
        stelModel.vertices = new Vec3d[stelModel.triangleCount * 3];
        stelModel.texcoords = new Vec2f[stelModel.triangleCount * 3];
        stelModel.normals = new Vec3f[stelModel.triangleCount * 3];
        int i = 0;
        for (FaceList::iterator it2 = model.faces.begin(); it2 != model.faces.end(); it2++) {
            Face& face = *it2;
            int three = 0;
            for (RefList::iterator it3 = face.refs.begin(); it3 != face.refs.end() && three < 3; it3++) {
                Ref& ref = *it3;
                Vertex& vert = vertices[ref.v];
                stelModel.vertices[i] = Vec3d(vert.x, vert.y, vert.z);
                if (ref.normal) {
                    Vertex& norm = normals[ref.n];
                    stelModel.normals[i] = Vec3f(norm.x, norm.y, norm.z);
                } else {
                    stelModel.normals[i] = Vec3f(0.0f, 0.0f, 0.0f);
                }
                if (ref.texture) {
                    Texcoord& tex = texcoords[ref.t];
                    stelModel.texcoords[i] = Vec2f(tex.u, tex.v);
                } else {
                    stelModel.texcoords[i] = Vec2f(0.0f, 0.0f);
                }
                three++;
                i++;
            }
        }
        stelModels.push_back(stelModel);
        stelModel = StelModel();
    }
    return stelModels;
}

void OBJ::transform(Mat4d mat)
{
    minX = minY = minZ =  numeric_limits<float>::max();
    maxX = maxY = maxZ = -numeric_limits<float>::max();
    for (std::vector<Vertex>::iterator it = vertices.begin(); it != vertices.end(); it++)
    {
        Vec3d v = Vec3d(it->x, it->y, it->z);
        mat.transfo(v);
        *it = (Vertex) { v[0], v[1], v[2] };
        minX = min(it->x, minX);
        minY = min(it->y, minY);
        minZ = min(it->z, minZ);
        maxX = max(it->x, maxX);
        maxY = max(it->y, maxY);
        maxZ = max(it->z, maxZ);
    }
}
