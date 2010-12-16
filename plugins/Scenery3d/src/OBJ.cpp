#include "OBJ.hpp"

#include <fstream>
#include <sstream>
#include <iostream>

#include "Util.hpp"
#include "PathInfo.hpp"

using namespace std;

OBJ::OBJ( void )
    :loaded(false), vertices(), texcoords(), normals(), models()
{
}

OBJ::~OBJ( void )
{
}

void OBJ::load( const char* filename )
{
    basePath = string(PathInfo::getDirectory(filename));
    ifstream file(filename);
    string line;
    if (file.is_open()) {
        Model model;
        while (!file.eof()) {
            getline(file, line);
            vector<string> parts = splitStr(line, ' ');
            if (parts.size() > 0) {
                if (parts[0] == "v") { // vertex (x,y,z)
                    if (parts.size() >= 4) {
                        Vertex v;
                        v.x = parseFloat(parts[1]);
                        v.y = parseFloat(parts[2]);
                        v.z = parseFloat(parts[3]);
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
                        vn.x = parseFloat(parts[1]);
                        vn.y = parseFloat(parts[2]);
                        vn.z = parseFloat(parts[3]);
                        normals.push_back(vn);
                    }
                } else if (parts[0] == "g") { // polygon group
                    if (parts.size() > 1) {
                        if (model.faces.size() > 0) {
                            models.push_back(model);
                            model = Model();
                        }
                        model.name = parts[1];
                    }
                } else if (parts[0] == "usemtl") { // use material
                    if (parts.size() > 1) {
                        model.material = parts[1];
                        trim_right(model.material);
                        printf("usemtl %s.\n", model.material.c_str());
                    }
                } else if (parts[0] == "f") { // face
                    Face face;
                    for (unsigned int i=1; i<parts.size(); ++i) {
                        Ref ref;
                        vector<string> f = splitStr(parts[i], '/');
                        if (f.size() >= 2) {
                            ref.v = parseInt(f[0]) - 1;
                            ref.texture = false;
                            if (f[1].size() > 0) {
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
                    }
                } else if (parts[0] == "mtllib") {
                    if (parts.size() > 1) {
                        string path = basePath + parts[1];
                        trim_right(path);
                        mtlLib.load(path.c_str());
                        mtlLib.uploadTexturesGL();
                    }
                }
            }
        }
        if (model.faces.size() > 0) {
            models.push_back(model);
            model = Model();
        }
        loaded = true;
        cout << vertices.size() << " vertices." << endl;
        cout << normals.size() << " normals." << endl;
        cout << texcoords.size() << " texture coordinates." << endl;
        cout << models.size() << " models." << endl;
        cout << filename << " loaded." << endl;
    } else {
        cerr << "Couldn't open " << filename << endl;
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

void OBJ::makeStelArrays(Vec3d*& vertices, Vec3f*& texcoords, Vec3f*& normals)
{
}

