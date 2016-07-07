/*
 * Stellarium
 * Copyright (C) 2016 Florian Schaukowitsch
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#include "StelOBJ.hpp"
#include "StelUtils.hpp"

#include <QBuffer>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTextStream>

Q_LOGGING_CATEGORY(stelOBJ,"stel.OBJ")

StelOBJ::StelOBJ()
{

}

StelOBJ::~StelOBJ()
{

}

void StelOBJ::clear()
{
	m_objectMap.clear();
	m_objects.clear();
	m_materialMap.clear();
	m_materials.clear();
	m_indices.clear();
	m_vertices.clear();
}

bool StelOBJ::load(const QString& filename)
{
	qCDebug(stelOBJ)<<"Loading"<<filename;

	QElapsedTimer timer;
	timer.start();

	//construct base path
	QFileInfo fi(filename);

	//try to open the file
	QFile file(filename);
	if(!file.open(QIODevice::ReadOnly))
	{
		qCCritical(stelOBJ)<<"Could not open file"<<filename<<file.errorString();
		return false;
	}

	qCDebug(stelOBJ)<<"Opened file in"<<timer.restart()<<"ms";

	//check if this is a compressed file
	if(filename.endsWith(".gz"))
	{
		//uncompress into memory
		QByteArray data = StelUtils::uncompress(file);
		file.close();
		//check if decompressing was successful
		if(data.isEmpty())
		{
			qCCritical(stelOBJ)<<"Could not decompress file"<<filename;
			return false;
		}
		qCDebug(stelOBJ)<<"Decompressed in"<<timer.elapsed()<<"ms";

		//create and open a QBuffer for reading
		QBuffer buf(&data);
		buf.open(QIODevice::ReadOnly);

		//perform actual load
		return load(buf,fi.canonicalPath());
	}

	//perform actual load
	return load(file,fi.canonicalPath());
}

//macro to test out different ways of comparison and their performance
#define CMD_CMP(a) (cmd==QLatin1String(a))

//macro to increase a list by size one and return a reference to the last element
//used instead of append() to avoid memory copies
#define INC_LIST(a) (a.resize(a.size()+1), a.last())

bool StelOBJ::parseBool(const ParseParams &params, bool &out)
{
	if(params.size()<2)
	{
		qCCritical(stelOBJ)<<"Expected parameter for statement"<<params;
		return false;
	}
	if(params.size()>2)
	{
		qCWarning(stelOBJ)<<"Additional parameters ignored in statement"<<params;
	}


	const QStringRef& cmd = params.at(1);
	out = (CMD_CMP("1") || CMD_CMP("true") || CMD_CMP("TRUE") || CMD_CMP("yes") || CMD_CMP("YES"));

	return true;
}

bool StelOBJ::parseString(const ParseParams &params, QString &out)
{
	if(params.size()<2)
	{
		qCCritical(stelOBJ)<<"Expected parameter for statement"<<params;
		return false;
	}
	if(params.size()>2)
	{
		qCWarning(stelOBJ)<<"Additional parameters ignored in statement"<<params;
	}

	out = params.at(1).toString();
	return true;
}

bool StelOBJ::parseFloat(const ParseParams &params, float &out)
{
	if(params.size()<2)
	{
		qCCritical(stelOBJ)<<"Expected parameter for statement"<<params;
		return false;
	}
	if(params.size()>2)
	{
		qCWarning(stelOBJ)<<"Additional parameters ignored in statement"<<params;
	}

	bool ok;
	out = params.at(1).toFloat(&ok);
	return ok;
}

template <typename T>
bool StelOBJ::parseVec3(const ParseParams& params, T &out)
{
	if(params.size()<4)
	{
		qCCritical(stelOBJ)<<"Invalid Vec3f specification"<<params;
		return false;
	}
	if(params.size()>4)
	{
		qCWarning(stelOBJ)<<"Additional parameters ignored in statement"<<params;
	}

	bool ok = false;
	out[0] = params.at(1).toFloat(&ok);
	if(ok)
	{
		out[1] = params.at(2).toFloat(&ok);
		if(ok)
		{
			out[2] = params.at(3).toFloat(&ok);
			return true;
		}
	}

	qCCritical(stelOBJ)<<"Error parsing Vec3:"<<params;
	return false;
}

template <typename T>
bool StelOBJ::parseVec2(const ParseParams& params,T &out)
{
	if(params.size()<3)
	{
		qCCritical(stelOBJ)<<"Invalid Vec2f specification"<<params;
		return false;
	}
	if(params.size()>3)
	{
		qCWarning(stelOBJ)<<"Additional parameters ignored in statement"<<params;
	}

	bool ok = false;
	out[0] = params.at(1).toFloat(&ok);
	if(ok)
	{
		out[1] = params.at(2).toFloat(&ok);
		return true;
	}

	qCCritical(stelOBJ)<<"Error parsing Vec2:"<<params;
	return false;
}

StelOBJ::Object* StelOBJ::getCurrentObject(CurrentParserState &state)
{
	//if there is a current object, return this one
	if(state.currentObject)
		return state.currentObject;

	//create the default object
	Object& obj = INC_LIST(m_objects);
	obj.name = "<default object>";
	obj.isDefaultObject = true;
	m_objectMap.insert(obj.name, m_objects.size()-1);
	state.currentObject = &obj;
	return &obj;
}

StelOBJ::MaterialGroup* StelOBJ::getCurrentMaterialGroup(CurrentParserState &state)
{
	int matIdx = getCurrentMaterialIndex(state);
	//if there is a current material group, check if a new one must be created because the material changed
	if(state.currentMaterialGroup && state.currentMaterialGroup->materialIndex==matIdx)
		return state.currentMaterialGroup;

	//no material group has been created yet
	//or the material has changed
	//we need to create a new group
	//we need an object for this
	Object* curObj = getCurrentObject(state);

	MaterialGroup& grp = INC_LIST(curObj->groups);
	grp.materialIndex = matIdx;
	//the object should always be the most recently added one
	grp.objectIndex = m_objects.size()-1;
	//the start index is positioned after the end of the index list
	grp.startIndex = m_indices.size();
	state.currentMaterialGroup = &grp;
	return &grp;
}

int StelOBJ::getCurrentMaterialIndex(CurrentParserState &state)
{
	//if there has been a material definition before, we use this
	if(m_materials.size())
		return state.currentMaterialIdx;

	//only if no material has been defined before any face,
	//we need to create a default material
	//this is "a white material" according to http://paulbourke.net/dataformats/obj/
	Material& mat = INC_LIST(m_materials);
	mat.name = "<default material>";
	mat.Kd = QVector3D(0.8f, 0.8f, 0.8f);
	mat.Ka = QVector3D(0.1f, 0.1f, 0.1f);

	m_materialMap.insert(mat.name, m_materials.size()-1);
	state.currentMaterialIdx = 0;
	return 0;
}

bool StelOBJ::parseFace(const ParseParams& params, const V3Vec& posList, const V3Vec& normList, const V2Vec& texList,
			CurrentParserState& state,
			VertexCache& vertCache)
{
	//The face definition can have 4 different variants
	//Mode 1: Only position:		f v1 v2 v3
	//Mode 2: Position+texcoords:		f v1/t1 v2/t2 v3/t3
	//Mode 3: Position+texcoords+normals:	f v1/t1/n1 v2/t2/n2 v3/t3/n3
	//Mode 4: Position+normals:		f v1//n1 v2//n2 v3//n3

	// Zero is actually invalid in the face definition, so we use it for default values
	int posIdx=0, texIdx=0, normIdx=0;
	// Contains the vertex indices
	QVarLengthArray<unsigned int,16> vIdx;

	if(params.size()<4)
	{
		qCCritical(stelOBJ)<<"Invalid number of vertices in face statement"<<params;
		return false;
	}

	int vtxAmount = params.size()-1;

	//parse each one seperately
	int mode = 0;
	bool ok = false;
	//a macro for consistency check
	#define CHK_MODE(a) if(mode && mode!=a) { qCCritical(stelOBJ)<<"Inconsistent face statement"<<params; return false; } else {mode = a;}
	//a macro for checking number pasing
	#define CHK_OK(a) do{ a; if(!ok) { qCCritical(stelOBJ)<<"Could not parse number in face statement"<<params; return false; } } while(0)
	//negative indices indicate relative data, i.e. -1 would mean the last position/texture/normal that was parsed
	//this macro fixes it up so that it always uses absolute numbers
	//note: the indices start with 1, this is fixed up later
	#define FIX_REL(a, list) if(a<0) {a += list.size()+1; }

	//loop to parse each section seperately
	for(int i =0; i<vtxAmount;++i)
	{
		//split on slash
		QVector<QStringRef> split = params.at(i+1).split('/');
		switch(split.size())
		{
			case 1: //no slash, only position
				CHK_MODE(1);
				CHK_OK(posIdx = split.at(0).toInt(&ok));
				FIX_REL(posIdx, posList);
				break;
			case 2: //single slash, vert/tex
				CHK_MODE(2);
				CHK_OK(posIdx = split.at(0).toInt(&ok));
				FIX_REL(posIdx, posList);
				CHK_OK(texIdx = split.at(1).toInt(&ok));
				FIX_REL(texIdx, texList);
				break;
			case 3: //2 slashes, either v/t/n or v//n
				if(!split.at(1).isEmpty())
				{
					CHK_MODE(3);
					CHK_OK(posIdx = split.at(0).toInt(&ok));
					FIX_REL(posIdx, posList);
					CHK_OK(texIdx = split.at(1).toInt(&ok));
					FIX_REL(texIdx, texList);
					CHK_OK(normIdx = split.at(2).toInt(&ok));
					FIX_REL(normIdx, normList);
				}
				else
				{
					CHK_MODE(4);
					CHK_OK(posIdx = split.at(0).toInt(&ok));
					FIX_REL(posIdx, posList);
					CHK_OK(normIdx = split.at(2).toInt(&ok));
					FIX_REL(normIdx, normList);
				}
				break;
			default: //invalid line
				qCCritical(stelOBJ)<<"Invalid face statement"<<params;
				return false;
		}

		//create a temporary Vertex by copying the info from the lists
		//zero initialize!
		Vertex v = Vertex();
		if(posIdx)
		{
			const float* data = posList.at(posIdx-1).v;
			std::copy(data, data+3, v.position);
		}
		if(texIdx)
		{
			const float* data = texList.at(texIdx-1).v;
			std::copy(data, data+2, v.texCoord);
		}
		if(normIdx)
		{
			const float* data = normList.at(normIdx-1).v;
			std::copy(data, data+3, v.normal);
		}

		//check if the vertex is already in the vertex cache
		if(vertCache.contains(v))
		{
			//cache hit, reuse index
			vIdx.append(vertCache.value(v));
		}
		else
		{
			//vertex unknown, add it to the vertex list and cache
			unsigned int idx = m_vertices.size();
			vertCache.insert(v,idx);
			m_vertices.append(v);
			vIdx.append(idx);
		}
	}

	//get/create current material group
	MaterialGroup* grp = getCurrentMaterialGroup(state);

	//vertex data has been loaded, create the faces
	//we use triangle-fan triangulation
	for(int i=2;i<vtxAmount;++i)
	{
		//the first one is always the same
		m_indices.append(vIdx[0]);
		m_indices.append(vIdx[i-1]);
		m_indices.append(vIdx[i]);
		//add the triangle to the group
		grp->indexCount+=3;
	}

	return true;
}

StelOBJ::MaterialList StelOBJ::Material::loadFromFile(const QString &filename)
{
	StelOBJ::MaterialList list;

	QFile file(filename);
	if(!file.open(QIODevice::ReadOnly))
	{
		qCWarning(stelOBJ)<<"Could not open MTL file"<<filename<<file.errorString();
		return list;
	}

	QTextStream stream(&file);
	Material* curMaterial = NULL;
	int lineNr = 0;

	while(!stream.atEnd())
	{
		++lineNr;
		bool ok = true;
		QString line = stream.readLine();
		//split line by space
		QVector<QStringRef> splits = line.splitRef(' ',QString::SkipEmptyParts);
		if(!splits.isEmpty())
		{
			const QStringRef& cmd = splits.at(0);

			//macro to make sure a material is currently active
			#define CHECK_MTL(a) do{ if(curMaterial) { a; } else { ok = false; qCCritical(stelOBJ)<<"Encountered material statement without active material"; } } while(0)
			if(CMD_CMP("newmtl")) //define new material
			{
				ok = splits.size()==2;
				if(ok)
				{
					const QStringRef& name = splits.at(1);
					//add a new material with the specified name
					list.resize(list.size()+1);
					curMaterial = &list.last();
					curMaterial->name = name.toString();
					ok = !name.isEmpty();
				}
				if(!ok)
				{
					qCCritical(stelOBJ)<<"Invalid newmtl statement"<<line;
				}
			}
			else if(CMD_CMP("Ka")) //define ambient color
			{
				CHECK_MTL(ok = parseVec3(splits,curMaterial->Ka));
			}
			else if(CMD_CMP("Kd")) //define diffuse color
			{
				CHECK_MTL(ok = parseVec3(splits,curMaterial->Kd));
			}
			else if(CMD_CMP("Ks")) //define specular color
			{
				CHECK_MTL(ok = parseVec3(splits,curMaterial->Ks));
			}
			else if(CMD_CMP("Ke")) //define emissive color
			{
				CHECK_MTL(ok = parseVec3(splits,curMaterial->Ke));
			}
			else if(CMD_CMP("Ns")) //define specular coefficient
			{
				CHECK_MTL(ok = parseFloat(splits,curMaterial->Ns));
			}
			else if(CMD_CMP("map_Ka")) //define ambient map
			{
				CHECK_MTL(ok = parseString(splits,curMaterial->map_Ka));
			}
			else if(CMD_CMP("map_Kd")) //define diffuse map
			{
				CHECK_MTL(ok = parseString(splits,curMaterial->map_Kd));
			}
			else if(CMD_CMP("map_Ks")) //define specular map
			{
				CHECK_MTL(ok = parseString(splits,curMaterial->map_Ks));
			}
			else if(CMD_CMP("map_Ke")) //define emissive map
			{
				CHECK_MTL(ok = parseString(splits,curMaterial->map_Ke));
			}
			else if(CMD_CMP("map_bump")) //define bump/normal map
			{
				CHECK_MTL(ok = parseString(splits,curMaterial->map_bump));
			}
			else if(CMD_CMP("map_height")) //define height map
			{
				CHECK_MTL(ok = parseString(splits,curMaterial->map_height));
			}
			else if(CMD_CMP("bAlphatest"))
			{
				CHECK_MTL(ok = parseBool(splits,curMaterial->bAlphatest));
			}
			else if(CMD_CMP("bBackface"))
			{
				CHECK_MTL(ok = parseBool(splits,curMaterial->bBackface));
			}
			else if(!cmd.startsWith("#"))
			{
				//unknown command, warn
				qCWarning(stelOBJ)<<"Unknown MTL statement:"<<line;
			}
		}

		if(!ok)
		{
			list.clear();
			qCCritical(stelOBJ)<<"Critical error in MTL file"<<filename<<"at line"<<lineNr<<", cannot process: "<<line;
			break;
		}
	}

	return list;
}

bool StelOBJ::load(QIODevice& device, const QString &basePath)
{
	clear();

	QDir baseDir(basePath);

	QElapsedTimer timer;
	timer.start();
	QTextStream stream(&device);

	//contains the parsed vertex positions
	V3Vec posList;
	//contains the parsed normals
	V3Vec normalList;
	//contains the parsed texture coords
	V2Vec texList;

	VertexCache vertCache;
	CurrentParserState state = CurrentParserState();

	int lineNr=0;

	//read file line by line
	while(!stream.atEnd())
	{
		++lineNr;
		QString line = stream.readLine();

		//split line by space
		QVector<QStringRef> splits = line.splitRef(' ',QString::SkipEmptyParts);
		if(!splits.isEmpty())
		{
			const QStringRef& cmd = splits.at(0);

			bool ok = true;

			if(CMD_CMP("f"))
			{
				ok = parseFace(splits,posList,normalList,texList,state,vertCache);
			}
			else if(CMD_CMP("v"))
			{
				ok = parseVec3(splits,INC_LIST(posList));
			}
			else if(CMD_CMP("vt"))
			{
				ok = parseVec2(splits,INC_LIST(texList));
			}
			else if(CMD_CMP("vn"))
			{
				ok = parseVec3(splits,INC_LIST(normalList));
			}
			else if(CMD_CMP("usemtl"))
			{
				ok = splits.size()==2;
				if(ok)
				{
					QString mtl = splits.at(1).toString();
					if(m_materialMap.contains(mtl))
					{
						//set material as active
						state.currentMaterialIdx = m_materialMap.value(mtl);
					}
					else
					{
						ok = false;
						qCCritical(stelOBJ)<<"Unknown material"<<mtl<<"has been referenced";
					}
				}
			}
			else if(CMD_CMP("mtllib"))
			{
				ok = splits.size()==2;
				if(ok)
				{
					//load external material file
					MaterialList newMaterials = Material::loadFromFile(baseDir.absoluteFilePath(splits.at(1).toString()));
					foreach(const Material& m, newMaterials)
					{
						m_materials.append(m);
						//the map has the index of the material
						//because pointers may change during parsing
						//because of list resizeing
						m_materialMap.insert(m.name,m_materials.size()-1);
					}
					qCDebug(stelOBJ)<<newMaterials.size()<<"loaded from MTL file"<<splits.at(1);
				}
			}
			else if(CMD_CMP("o"))
			{
				//create new object
				Object& obj = INC_LIST(m_objects);
				ok = parseString(splits, obj.name);
				if(ok)
				{
					m_objectMap.insert(obj.name,m_objects.size()-1);
					state.currentObject = &obj;
					//also clear material group to make sure a new group is created
					state.currentMaterialGroup = NULL;
				}
			}
			else if(!cmd.startsWith('#'))
			{
				//unknown command, warn
				qCWarning(stelOBJ)<<"Unknown OBJ statement:"<<line;
			}

			if(!ok)
			{
				qCCritical(stelOBJ)<<"Critical error on OBJ line"<<lineNr<<", cannot load OBJ data: "<<line;
				return false;
			}
		}
	}

	//finished loading, squeeze the arrays to save memory
	m_vertices.squeeze();
	m_indices.squeeze();

	qCDebug(stelOBJ)<<"Loaded OBJ in"<<timer.elapsed()<<"ms";
	qCDebug(stelOBJ, "Parsed %d positions, %d normals, %d texture coordinates, %d materials",
		posList.size(), normalList.size(), texList.size(), m_materials.size());
	qCDebug(stelOBJ, "Created %d vertices, %d faces, %d objects", m_vertices.size(), getFaceCount(), m_objects.size());


	device.close();
	return true;
}
