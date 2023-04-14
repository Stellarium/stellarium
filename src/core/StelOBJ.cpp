/*
 * Stellarium
 * Copyright (C) 2012 Andrei Borza
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

//#include "StelApp.hpp"
#include "StelOBJ.hpp"
//#include "StelTextureMgr.hpp"
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
	: m_isLoaded(false)
{
}

StelOBJ::~StelOBJ()
{
}

void StelOBJ::clear()
{
	//just create a new object
	*this = StelOBJ();
}

bool StelOBJ::load(const QString& filename, const VertexOrder vertexOrder, const bool forceCreateNormals)
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
		return load(buf,fi.canonicalPath(),vertexOrder, forceCreateNormals);
	}

	//perform actual load
	return load(file,fi.canonicalPath(),vertexOrder, forceCreateNormals);
}

//macro to test out different ways of comparison and their performance
#define CMD_CMP(a) (QLatin1String(a)==cmd)

//macro to increase a list by size one and return a reference to the last element
//used instead of append() to avoid memory copies
#define INC_LIST(a) (a.resize(a.size()+1), a.last())

bool StelOBJ::parseBool(const ParseParams &params, bool &out, int paramsStart)
{
	if(params.size()-paramsStart<1)
	{
		qCCritical(stelOBJ)<<"Expected parameter for statement"<<params;
		return false;
	}
	if(params.size()-paramsStart>1)
	{
		qCWarning(stelOBJ)<<"Additional parameters ignored in statement"<<params;
	}


	const ParseParam& cmd = params.at(paramsStart);
	out = (CMD_CMP("1") || CMD_CMP("true") || CMD_CMP("TRUE") || CMD_CMP("yes") || CMD_CMP("YES"));

	return true;
}

bool StelOBJ::parseInt(const ParseParams &params, int &out, int paramsStart)
{
	if(params.size()-paramsStart<1)
	{
		qCCritical(stelOBJ)<<"Expected parameter for statement"<<params;
		out=0;
		return false;
	}
	if(params.size()-paramsStart>1)
	{
		qCWarning(stelOBJ)<<"Additional parameters ignored in statement"<<params;
	}

	bool ok;
	out = params.at(paramsStart).toInt(&ok);
	return ok;
}

bool StelOBJ::parseString(const ParseParams &params, QString &out, int paramsStart)
{
	if(params.size()-paramsStart<1)
	{
		qCCritical(stelOBJ)<<"Expected parameter for statement"<<params;
		return false;
	}
	if(params.size()-paramsStart>1)
	{
		qCWarning(stelOBJ)<<"Additional parameters ignored in statement"<<params;
	}

	out = params.at(paramsStart).toString();
	return true;
}

QString StelOBJ::getRestOfString(const QString &strip, const QString &line)
{
	return line.mid(strip.length()).trimmed();
}

bool StelOBJ::parseFloat(const ParseParams &params, float &out, int paramsStart)
{
	if(params.size()-paramsStart<1)
	{
		qCCritical(stelOBJ)<<"Expected parameter for statement"<<params;
		return false;
	}
	if(params.size()-paramsStart>1)
	{
		qCWarning(stelOBJ)<<"Additional parameters ignored in statement"<<params;
	}

	bool ok;
	out = params.at(paramsStart).toFloat(&ok);
	return ok;
}

template <typename T>
bool StelOBJ::parseVec3(const ParseParams& params, T &out, int paramsStart)
{
	if(params.size()-paramsStart<3)
	{
		qCCritical(stelOBJ)<<"Invalid Vec3f specification"<<params;
		return false;
	}

	bool ok = false;
	out[0] = params.at(paramsStart).toFloat(&ok); //use double here, so that it even works for Vec3d, etc
	if(ok)
	{
		out[1] = params.at(paramsStart+1).toFloat(&ok);
		if(ok)
		{
			out[2] = params.at(paramsStart+2).toFloat(&ok);
			return true;
		}
	}

	qCCritical(stelOBJ)<<"Error parsing Vec3:"<<params;
	return false;
}

template <typename T>
bool StelOBJ::parseVec2(const ParseParams& params,T &out, int paramsStart)
{
	if(params.size()-paramsStart<2)
	{
		qCCritical(stelOBJ)<<"Invalid Vec2f specification"<<params;
		return false;
	}

	bool ok = false;
	out[0] = params.at(paramsStart).toDouble(&ok);
	if(ok)
	{
		out[1] = params.at(paramsStart+1).toDouble(&ok);
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

	//parse each one separately
	int mode = 0;
	bool ok = false;
	//a macro for consistency check
	#define CHK_MODE(a) if(mode && mode!=a) { qCCritical(stelOBJ)<<"Inconsistent face statement"<<params; return false; } else {mode = a;}
	//a macro for checking number parsing
	#define CHK_OK(a) do{ a; if(!ok) { qCCritical(stelOBJ)<<"Could not parse number in face statement"<<params; return false; } } while(0)
	//negative indices indicate relative data, i.e. -1 would mean the last position/texture/normal that was parsed
	//this macro fixes it up so that it always uses absolute numbers
	//note: the indices start with 1, this is fixed up later
	#define FIX_REL(a, list) if(a<0) {a += list.size()+1; }

	//loop to parse each section separately
	for(int i =0; i<vtxAmount;++i)
	{
		//split on slash
		#if (QT_VERSION>=QT_VERSION_CHECK(6,0,0))
		ParseParams split = params.at(i+1).split('/').toVector();
		#else
		ParseParams split = params.at(i+1).split('/');
		#endif
		switch(split.size())
		{
			case 1: //no slash, only position
				CHK_MODE(1)
				CHK_OK(posIdx = split.at(0).toInt(&ok));
				FIX_REL(posIdx, posList)
				break;
			case 2: //single slash, vert/tex
				CHK_MODE(2)
				CHK_OK(posIdx = split.at(0).toInt(&ok));
				FIX_REL(posIdx, posList)
				CHK_OK(texIdx = split.at(1).toInt(&ok));
				FIX_REL(texIdx, texList)
				break;
			case 3: //2 slashes, either v/t/n or v//n
				if(!split.at(1).isEmpty())
				{
					CHK_MODE(3)
					CHK_OK(posIdx = split.at(0).toInt(&ok));
					FIX_REL(posIdx, posList)
					CHK_OK(texIdx = split.at(1).toInt(&ok));
					FIX_REL(texIdx, texList)
					CHK_OK(normIdx = split.at(2).toInt(&ok));
					FIX_REL(normIdx, normList)
				}
				else
				{
					CHK_MODE(4)
					CHK_OK(posIdx = split.at(0).toInt(&ok));
					FIX_REL(posIdx, posList)
					CHK_OK(normIdx = split.at(2).toInt(&ok));
					FIX_REL(normIdx, normList)
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
		auto it = vertCache.find(v);
		if(it!=vertCache.end())
		{
			//cache hit, reuse index
			vIdx.append(*it);
		}
		else
		{
			//vertex unknown, add it to the vertex list and cache
			unsigned int idx = static_cast<unsigned int>(m_vertices.size());
			vertCache.insert(v,m_vertices.size());
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

	QFileInfo fi(filename);
	QDir dir = fi.dir();
	QFile file(filename);
	if(!file.open(QIODevice::ReadOnly))
	{
		qCWarning(stelOBJ)<<"Could not open MTL file"<<filename<<file.errorString();
		return list;
	}

	QTextStream stream(&file);
	Material* curMaterial = Q_NULLPTR;
	int lineNr = 0;
	// some exporters give d and Tr, and some give contradicting interpretations. Track a warning with these.
	bool dHasBeenGiven = false;
	bool trHasBeenGiven = false;

	while(!stream.atEnd())
	{
		++lineNr;
		bool ok = true;
		//make sure only spaces are the separator
		QString line = stream.readLine().simplified();
		//split line by space		
		#if (QT_VERSION>=QT_VERSION_CHECK(6,0,0))
		ParseParams splits = ParseParam(line).split(' ', Qt::SkipEmptyParts).toVector();
		#elif (QT_VERSION>=QT_VERSION_CHECK(5,15,0))
		ParseParams splits = line.splitRef(' ', Qt::SkipEmptyParts);
		#else
		ParseParams splits = line.splitRef(' ', QString::SkipEmptyParts);
		#endif
		if(!splits.isEmpty())
		{
			const ParseParam& cmd = splits.at(0);

			//macro to make sure a material is currently active
			#define CHECK_MTL() if(!curMaterial) { ok = false; qCCritical(stelOBJ)<<"Encountered material statement without active material"; }
			//macro to make path absolute, also to force use of forward slashes
			#define MAKE_ABS(a) if(!a.isEmpty()){ a = dir.absoluteFilePath(QDir::cleanPath(a.replace('\\','/'))); }
			if(CMD_CMP("newmtl")) //define new material
			{
				//use rest of line to support spaces in file name
				QString name = getRestOfString(QStringLiteral("newmtl"),line);
				ok = !name.isEmpty();
				if(ok)
				{
					//add a new material with the specified name
					curMaterial = &INC_LIST(list);
					curMaterial->name = name;
					dHasBeenGiven = false;
					trHasBeenGiven = false;
				}
				else
				{
					qCCritical(stelOBJ)<<"Invalid newmtl statement"<<line;
				}
			}
			else if(CMD_CMP("Ka")) //define ambient color
			{
				CHECK_MTL()
				if(ok)
					ok = parseVec3(splits,curMaterial->Ka);
			}
			else if(CMD_CMP("Kd")) //define diffuse color
			{
				CHECK_MTL()
				if(ok)
					ok = parseVec3(splits,curMaterial->Kd);
			}
			else if(CMD_CMP("Ks")) //define specular color
			{
				CHECK_MTL()
				if(ok)
					ok = parseVec3(splits,curMaterial->Ks);
			}
			else if(CMD_CMP("Ke")) //define emissive color
			{
				CHECK_MTL()
				if(ok)
					ok = parseVec3(splits,curMaterial->Ke);
			}
			else if(CMD_CMP("Ns")) //define specular coefficient
			{
				CHECK_MTL()
				if(ok)
					ok = StelOBJ::parseFloat(splits,curMaterial->Ns);
			}
			else if(CMD_CMP("d"))
			{
				CHECK_MTL()
				if(ok)
				{
					ok = StelOBJ::parseFloat(splits,curMaterial->d);
					//clamp d to [0,1]
					curMaterial->d = qBound(0.0f, curMaterial->d, 1.0f);
					if (trHasBeenGiven)
					{
						qWarning(stelOBJ) << "Material" << curMaterial->name << "warning: Tr and d both given. The latter wins, d=" << curMaterial->d;
					}
					dHasBeenGiven=true;
				}
			}
			else if(CMD_CMP("Tr"))
			{
				CHECK_MTL()
				if(ok)
				{
					//Tr should be the inverse of d, in theory
					//not all exporters seem to follow this rule...
					ok = StelOBJ::parseFloat(splits,curMaterial->d);
					//clamp d to [0,1]
					curMaterial->d = 1.0f - qBound(0.0f, curMaterial->d, 1.0f);
					if (dHasBeenGiven)
					{
						qWarning(stelOBJ) << "Material" << curMaterial->name << "warning: d and Tr both given. The latter wins, Tr=" << 1.0f-curMaterial->d;
					}
					trHasBeenGiven=true;
				}
			}
			else if(CMD_CMP("map_Ka")) //define ambient map
			{
				CHECK_MTL()
				if(ok)
				{
					//use rest of line to support spaces in file name
					curMaterial->map_Ka = getRestOfString(QStringLiteral("map_Ka"),line);
					ok = !curMaterial->map_Ka.isEmpty();
					MAKE_ABS(curMaterial->map_Ka)
				}
			}
			else if(CMD_CMP("map_Kd")) //define diffuse map
			{
				CHECK_MTL()
				if(ok)
				{
					//use rest of line to support spaces in file name
					curMaterial->map_Kd = getRestOfString(QStringLiteral("map_Kd"),line);
					ok = !curMaterial->map_Kd.isEmpty();
					MAKE_ABS(curMaterial->map_Kd)
				}
			}
			else if(CMD_CMP("map_Ks")) //define specular map
			{
				CHECK_MTL()
				if(ok)
				{
					//use rest of line to support spaces in file name
					curMaterial->map_Ks = getRestOfString(QStringLiteral("map_Ks"),line);
					ok = !curMaterial->map_Ks.isEmpty();
					MAKE_ABS(curMaterial->map_Ks)
				}
			}
			else if(CMD_CMP("map_Ke")) //define emissive map
			{
				CHECK_MTL()
				if(ok)
				{
					//use rest of line to support spaces in file name
					curMaterial->map_Ke = getRestOfString(QStringLiteral("map_Ke"),line);
					ok = !curMaterial->map_Ke.isEmpty();
					MAKE_ABS(curMaterial->map_Ke)
				}
			}
			else if(CMD_CMP("map_bump")) //define bump/normal map
			{
				CHECK_MTL()
				if(ok)
				{
					//use rest of line to support spaces in file name
					curMaterial->map_bump = getRestOfString(QStringLiteral("map_bump"),line);
					ok = !curMaterial->map_bump.isEmpty();
					MAKE_ABS(curMaterial->map_bump)
				}
			}
			else if(CMD_CMP("map_height")) //define height map
			{
				CHECK_MTL()
				if(ok)
				{
					//use rest of line to support spaces in file name
					curMaterial->map_height = getRestOfString(QStringLiteral("map_height"),line);
					ok = !curMaterial->map_height.isEmpty();
					MAKE_ABS(curMaterial->map_height)
				}
			}
			else if(CMD_CMP("illum"))
			{
				CHECK_MTL()
				if(ok)
				{
					int tmp;
					ok = parseInt(splits,tmp);
					curMaterial->illum = static_cast<Illum>(tmp);

					if(tmp<I_DIFFUSE || tmp > I_TRANSLUCENT)
					{
						ok = false;
						tmp = I_NONE;
						qCCritical(stelOBJ())<<"Invalid illum statement"<<line;
					}

					//if between these 2, set to translucent and warn
					if(tmp>I_SPECULAR && tmp < I_TRANSLUCENT)
					{
						qCWarning(stelOBJ())<<"Treating illum "<<tmp<<"as TRANSLUCENT";
						tmp = I_TRANSLUCENT;
					}
					curMaterial->illum = static_cast<Illum>(tmp);
				}
			}
			else if(!cmd.startsWith(QChar('#')))
			{
				CHECK_MTL()
				if(ok)
				{
					//unknown command, add to additional params
					//we need to convert to actual string instances to store them
					QStringList list;
					for(int i = 1; i<splits.size();++i)
					{
						list.append(splits.at(i).toString());
					}

					curMaterial->additionalParams.insert(cmd.toString(),list);
					//qCWarning(stelOBJ)<<"Unknown MTL statement:"<<line;
				}
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

bool StelOBJ::Material::parseBool(const QStringList &params, bool &out)
{
	ParseParams pp(params.size());
	for(int i = 0; i< params.size();++i)
	{
#if (QT_VERSION>=QT_VERSION_CHECK(6,0,0))
		pp[i] = params.at(i);
#else
		pp[i] = QStringRef(&params.at(i));
#endif
	}
	return StelOBJ::parseBool(pp,out,0);
}

bool StelOBJ::Material::parseFloat(const QStringList &params, float &out)
{
	ParseParams pp(params.size());
	for(int i = 0; i< params.size();++i)
	{
#if (QT_VERSION>=QT_VERSION_CHECK(6,0,0))
		pp[i] = params.at(i);
#else
		pp[i] = ParseParam(&params.at(i));
#endif
	}
	return StelOBJ::parseFloat(pp,out,0);
}

bool StelOBJ::Material::parseVec2d(const QStringList &params, Vec2d &out)
{
	ParseParams pp(params.size());
	for(int i = 0; i< params.size();++i)
	{
#if (QT_VERSION>=QT_VERSION_CHECK(6,0,0))
		pp[i] = params.at(i);
#else
		pp[i] = ParseParam(&params.at(i));
#endif
	}
	return StelOBJ::parseVec2(pp,out,0);
}

void StelOBJ::addObject(const QString &name, CurrentParserState &state)
{
	//check if the last object contained anything, if not remove it
	if(state.currentObject)
	{
		if(state.currentObject->groups.isEmpty())
		{
			Q_ASSERT(state.currentObject == &m_objects.last());
			m_objectMap.remove(state.currentObject->name);
			m_objects.removeLast();
		}
	}

	//create new object
	Object& obj = INC_LIST(m_objects);
	obj.name = name;
	m_objectMap.insert(obj.name,m_objects.size()-1);
	state.currentObject = &obj;
	//also clear material group to make sure a new group is created
	state.currentMaterialGroup = Q_NULLPTR;
}

bool StelOBJ::load(QIODevice& device, const QString &basePath, const VertexOrder vertexOrder, const bool forceCreateNormals)
{
	clear();

	QDir baseDir(basePath);

	QElapsedTimer timer;
	timer.start();
	QTextStream stream(&device);

	bool smoothGroupWarned = false;
	bool vertexWWarned = false;
	bool textureWWarned = false;

	//contains the parsed vertex positions
	V3Vec posList;
	//contains the parsed normals
	V3Vec normalList;
	//contains the parsed texture coords
	V2Vec texList;

	VertexCache vertCache;
	CurrentParserState state = CurrentParserState();
	static const QRegularExpression separator("\\s");
	separator.optimize();

	int lineNr=0;

	//read file line by line
	while(!stream.atEnd())
	{
		++lineNr;
		//ignore front/back whitespace
		QString line = stream.readLine().trimmed();

		//split line by whitespace
		#if (QT_VERSION>=QT_VERSION_CHECK(6,0,0))
		ParseParams splits = ParseParam(line).split(separator, Qt::SkipEmptyParts).toVector();
		#elif (QT_VERSION>=QT_VERSION_CHECK(5,15,0))
		ParseParams splits = line.splitRef(separator, Qt::SkipEmptyParts);
		#else
		ParseParams splits = line.splitRef(separator, QString::SkipEmptyParts);
		#endif
		if(!splits.isEmpty())
		{
			const ParseParam& cmd = splits.at(0);

			bool ok = true;

			if(CMD_CMP("f"))
			{
				ok = parseFace(splits,posList,normalList,texList,state,vertCache);
			}
			else if(CMD_CMP("v"))
			{
				//we have to handle the vertex order
				Vec3f& target = INC_LIST(posList);
				ok = parseVec3(splits,target);
				//check the optional w coord if we have a vec4, must be 1
				if(splits.size()>4)
				{
					float w;
					parseFloat(splits,w,4);
					if((!qFuzzyCompare(w,1.0f)) && (!vertexWWarned))
					{
						qWarning(stelOBJ)<<"Vertex w coordinates different from 1.0 are not supported, changed to 1.0, starting on line"<<lineNr;
						vertexWWarned=true;
					}
				}
				switch(vertexOrder)
				{
					case XYZ:
						//no change
						break;
					case XZY:
						target.set(target[0],-target[2],target[1]);
						break;
					case YXZ:
						target.set(target[1],target[0],target[2]);
						break;
					case YZX:
						target.set(target[1],target[2],target[0]);
						break;
					case ZXY:
						target.set(target[2],target[0],target[1]);
						break;
					case ZYX:
						target.set(target[2],target[1],target[0]);
						break;
					default:
						Q_ASSERT_X(0,"StelOBJ::load","invalid vertex order found");
						qCWarning(stelOBJ) << "Vertex order"<<vertexOrder<<"not implemented, assuming XYZ";
						break;
				}
			}
			else if(CMD_CMP("vt"))
			{
				ok = parseVec2(splits,INC_LIST(texList));
				//check the optional w coord if we have a vec3, must be 0
				if(splits.size()>3)
				{
					float w;
					parseFloat(splits,w,3);
					if( (!qFuzzyIsNull(w)) && (!textureWWarned))
					{
						qWarning(stelOBJ)<<"Texture w coordinates are not supported, starting on line"<<lineNr;
						textureWWarned=true;
					}
				}
			}
			else if(CMD_CMP("vn"))
			{
				//we have to handle the vertex order
				Vec3f& target = INC_LIST(normalList);
				ok = parseVec3(splits,target);
				switch(vertexOrder)
				{
					case XYZ:
						//no change
						break;
					case XZY:
						target.set(target[0],-target[2],target[1]);
						break;
					case YXZ:
						target.set(target[1],target[0],target[2]);
						break;
					case YZX:
						target.set(target[1],target[2],target[0]);
						break;
					case ZXY:
						target.set(target[2],target[0],target[1]);
						break;
					case ZYX:
						target.set(target[2],target[1],target[0]);
						break;
					default:
						Q_ASSERT_X(0,"StelOBJ::load","invalid vertex order found");
						qCWarning(stelOBJ) << "Vertex order"<<vertexOrder<<"not implemented, assuming XYZ";
						break;
				}
				//normalize is usually not needed so we skip it
				//target.normalize();
			}
			else if(CMD_CMP("usemtl"))
			{
				//use the rest of the string
				QString mtl = getRestOfString(QStringLiteral("usemtl"),line);
				ok = !mtl.isEmpty();
				if(ok)
				{
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
				else
					qCCritical(stelOBJ)<<"No material name given";
			}
			else if(CMD_CMP("mtllib"))
			{
				//use the rest of the string
				QString fileName = getRestOfString(QStringLiteral("mtllib"),line);
				ok = !fileName.isEmpty();
				if(ok)
				{
					//load external material file
					const MaterialList newMaterials = Material::loadFromFile(baseDir.absoluteFilePath(fileName));
					for (const auto& m : newMaterials)
					{
						m_materials.append(m);
						//the map has the index of the material
						//because pointers may change during parsing
						//because of list resizeing
						m_materialMap.insert(m.name,m_materials.size()-1);
					}
					qCDebug(stelOBJ)<<newMaterials.size()<<"materials loaded from MTL file"<<fileName;
				}
				else
					qCCritical(stelOBJ)<<"No material file name given";
			}
			else if(CMD_CMP("o"))
			{
				//use the rest of the string
				QString objName = getRestOfString(QStringLiteral("o"),line);
				ok = !objName.isEmpty();
				if(ok)
				{
					addObject(objName, state);
				}
				else
					qCCritical(stelOBJ)<<"Object name is required";
			}
			else if(CMD_CMP("g"))
			{
				//use the rest of the string
				QString objName = getRestOfString(QStringLiteral("g"),line);
				ok = !objName.isEmpty();
				if(ok)
				{
					addObject(objName, state);
				}
				else
					qCCritical(stelOBJ)<<"Group name is required";
			}
			else if(CMD_CMP("s"))
			{
				if(!smoothGroupWarned)
				{
					qCWarning(stelOBJ)<<"Smoothing groups are not supported, consider re-exporting your model from Blender";
					smoothGroupWarned = true;
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

	device.close();

	//finished loading, squeeze the arrays to save some memory
	m_vertices.squeeze();
	m_indices.squeeze();

	Q_ASSERT(m_indices.size() % 3 == 0);

	qCDebug(stelOBJ)<<"Loaded OBJ in"<<timer.elapsed()<<"ms";
	qCDebug(stelOBJ, "Parsed %d positions, %d normals, %d texture coordinates, %d materials",
		int(posList.size()), int(normalList.size()), int(texList.size()), int(m_materials.size()));
	qCDebug(stelOBJ, "Created %d vertices, %d faces, %d objects", int(m_vertices.size()), getFaceCount(), int(m_objects.size()));

	//perform post processing
	performPostProcessing(normalList.isEmpty() || forceCreateNormals);
	m_isLoaded = true;
	return true;
}

void StelOBJ::Object::postprocess(const StelOBJ &obj, Vec3d &centroid)
{
	const VertexList& vList = obj.getVertexList();
	const IndexList& iList = obj.getIndexList();

	int idxCnt = 0;
	boundingbox.reset();

	//iterate through the groups
	for(int i =0;i<groups.size();++i)
	{
		MaterialGroup& grp = groups[i];
		Vec3d accVertex(0.);

		Q_ASSERT(grp.indexCount > 0);
		grp.boundingbox.reset();

		//iterate through the vertices of the group
		for(int idx = grp.startIndex;idx<(grp.startIndex+grp.indexCount);++idx)
		{
			const Vertex& v = vList.at(static_cast<int>(iList.at(idx)));
			Vec3f pos(v.position);
			grp.boundingbox.expand(pos);
			accVertex+=pos.toVec3d();
			centroid+=pos.toVec3d();
		}
		boundingbox.expand(grp.boundingbox);
		grp.centroid = (accVertex / grp.indexCount).toVec3f();

		idxCnt += grp.indexCount;
	}
	Q_ASSERT(idxCnt>0);
	//only do 1 division for more accuracy
	centroid /= idxCnt;
	this->centroid = centroid.toVec3f();
}

void StelOBJ::generateNormals()
{
	//Code adapted from old OBJ loader (Andrei Borza)

	const unsigned int *pTriangle = Q_NULLPTR;
	Vertex *pVertex0 = Q_NULLPTR;
	Vertex *pVertex1 = Q_NULLPTR;
	Vertex *pVertex2 = Q_NULLPTR;
	float edge1[3] = {0.0f, 0.0f, 0.0f};
	float edge2[3] = {0.0f, 0.0f, 0.0f};
	float normal[3] = {0.0f, 0.0f, 0.0f};
	float invlength = 0.0f;
	int totalVertices = m_vertices.size();
	int totalTriangles = m_indices.size() / 3;

	// Initialize all the vertex normals.
	for (int i=0; i<totalVertices; ++i)
	{
		pVertex0 = &m_vertices[i];
		pVertex0->normal[0] = 0.0f;
		pVertex0->normal[1] = 0.0f;
		pVertex0->normal[2] = 0.0f;
	}

	// Calculate the vertex normals.
	for (int i=0; i<totalTriangles; ++i)
	{
		pTriangle = &m_indices.at(i*3);

		pVertex0 = &m_vertices[static_cast<int>(pTriangle[0])];
		pVertex1 = &m_vertices[static_cast<int>(pTriangle[1])];
		pVertex2 = &m_vertices[static_cast<int>(pTriangle[2])];

		// Calculate triangle face normal.
		edge1[0] = static_cast<float>(pVertex1->position[0] - pVertex0->position[0]);
		edge1[1] = static_cast<float>(pVertex1->position[1] - pVertex0->position[1]);
		edge1[2] = static_cast<float>(pVertex1->position[2] - pVertex0->position[2]);

		edge2[0] = static_cast<float>(pVertex2->position[0] - pVertex0->position[0]);
		edge2[1] = static_cast<float>(pVertex2->position[1] - pVertex0->position[1]);
		edge2[2] = static_cast<float>(pVertex2->position[2] - pVertex0->position[2]);

		normal[0] = (edge1[1]*edge2[2]) - (edge1[2]*edge2[1]);
		normal[1] = (edge1[2]*edge2[0]) - (edge1[0]*edge2[2]);
		normal[2] = (edge1[0]*edge2[1]) - (edge1[1]*edge2[0]);

		// Accumulate the normals.

		pVertex0->normal[0] += normal[0];
		pVertex0->normal[1] += normal[1];
		pVertex0->normal[2] += normal[2];

		pVertex1->normal[0] += normal[0];
		pVertex1->normal[1] += normal[1];
		pVertex1->normal[2] += normal[2];

		pVertex2->normal[0] += normal[0];
		pVertex2->normal[1] += normal[1];
		pVertex2->normal[2] += normal[2];
	}

	// Normalize the vertex normals.
	for (int i=0; i<totalVertices; ++i)
	{
		pVertex0 = &m_vertices[i];

		invlength = 1.0f / std::sqrt(pVertex0->normal[0]*pVertex0->normal[0] +
				pVertex0->normal[1]*pVertex0->normal[1] +
				pVertex0->normal[2]*pVertex0->normal[2]);

		pVertex0->normal[0] *= invlength;
		pVertex0->normal[1] *= invlength;
		pVertex0->normal[2] *= invlength;
	}
}

void StelOBJ::generateTangents()
{
	//Code adapted from old OBJ loader (Andrei Borza)

	const unsigned int *pTriangle = Q_NULLPTR;
	Vertex *pVertex0 = Q_NULLPTR;
	Vertex *pVertex1 = Q_NULLPTR;
	Vertex *pVertex2 = Q_NULLPTR;
	float edge1[3] = {0.0f, 0.0f, 0.0f};
	float edge2[3] = {0.0f, 0.0f, 0.0f};
	float texEdge1[2] = {0.0f, 0.0f};
	float texEdge2[2] = {0.0f, 0.0f};
	float tangent[3] = {0.0f, 0.0f, 0.0f};
	float bitangent[3] = {0.0f, 0.0f, 0.0f};
	float det = 0.0f;
	float nDotT = 0.0f;
	float bDotB = 0.0f;
	float invlength = 0.0f;
	const int totalVertices = m_vertices.size();
	const int totalTriangles = m_indices.size() / 3;

	// Initialize all the vertex tangents and bitangents.
	for (int i=0; i<totalVertices; ++i)
	{
		pVertex0 = &m_vertices[i];

		pVertex0->tangent[0] = 0.0f;
		pVertex0->tangent[1] = 0.0f;
		pVertex0->tangent[2] = 0.0f;
		pVertex0->tangent[3] = 0.0f;

		pVertex0->bitangent[0] = 0.0f;
		pVertex0->bitangent[1] = 0.0f;
		pVertex0->bitangent[2] = 0.0f;
	}

	// Calculate the vertex tangents and bitangents.
	for (int i=0; i<totalTriangles; ++i)
	{
		pTriangle = &m_indices.at(i*3);

		pVertex0 = &m_vertices[static_cast<int>(pTriangle[0])];
		pVertex1 = &m_vertices[static_cast<int>(pTriangle[1])];
		pVertex2 = &m_vertices[static_cast<int>(pTriangle[2])];

		// Calculate the triangle face tangent and bitangent.

		edge1[0] = static_cast<float>(pVertex1->position[0] - pVertex0->position[0]);
		edge1[1] = static_cast<float>(pVertex1->position[1] - pVertex0->position[1]);
		edge1[2] = static_cast<float>(pVertex1->position[2] - pVertex0->position[2]);

		edge2[0] = static_cast<float>(pVertex2->position[0] - pVertex0->position[0]);
		edge2[1] = static_cast<float>(pVertex2->position[1] - pVertex0->position[1]);
		edge2[2] = static_cast<float>(pVertex2->position[2] - pVertex0->position[2]);

		texEdge1[0] = pVertex1->texCoord[0] - pVertex0->texCoord[0];
		texEdge1[1] = pVertex1->texCoord[1] - pVertex0->texCoord[1];

		texEdge2[0] = pVertex2->texCoord[0] - pVertex0->texCoord[0];
		texEdge2[1] = pVertex2->texCoord[1] - pVertex0->texCoord[1];

		det = texEdge1[0]*texEdge2[1] - texEdge2[0]*texEdge1[1];

		if (fabs(det) < 1e-6f)
		{
			tangent[0] = 1.0f;
			tangent[1] = 0.0f;
			tangent[2] = 0.0f;

			bitangent[0] = 0.0f;
			bitangent[1] = 1.0f;
			bitangent[2] = 0.0f;
		}
		else
		{
			det = 1.0f / det;

			tangent[0] = (texEdge2[1]*edge1[0] - texEdge1[1]*edge2[0])*det;
			tangent[1] = (texEdge2[1]*edge1[1] - texEdge1[1]*edge2[1])*det;
			tangent[2] = (texEdge2[1]*edge1[2] - texEdge1[1]*edge2[2])*det;

			bitangent[0] = (-texEdge2[0]*edge1[0] + texEdge1[0]*edge2[0])*det;
			bitangent[1] = (-texEdge2[0]*edge1[1] + texEdge1[0]*edge2[1])*det;
			bitangent[2] = (-texEdge2[0]*edge1[2] + texEdge1[0]*edge2[2])*det;
		}

		// Accumulate the tangents and bitangents.

		pVertex0->tangent[0] += tangent[0];
		pVertex0->tangent[1] += tangent[1];
		pVertex0->tangent[2] += tangent[2];
		pVertex0->bitangent[0] += bitangent[0];
		pVertex0->bitangent[1] += bitangent[1];
		pVertex0->bitangent[2] += bitangent[2];

		pVertex1->tangent[0] += tangent[0];
		pVertex1->tangent[1] += tangent[1];
		pVertex1->tangent[2] += tangent[2];
		pVertex1->bitangent[0] += bitangent[0];
		pVertex1->bitangent[1] += bitangent[1];
		pVertex1->bitangent[2] += bitangent[2];

		pVertex2->tangent[0] += tangent[0];
		pVertex2->tangent[1] += tangent[1];
		pVertex2->tangent[2] += tangent[2];
		pVertex2->bitangent[0] += bitangent[0];
		pVertex2->bitangent[1] += bitangent[1];
		pVertex2->bitangent[2] += bitangent[2];
	}

	// Orthogonalize and normalize the vertex tangents.
	for (int i=0; i<totalVertices; ++i)
	{
		pVertex0 = &m_vertices[i];

		// Gram-Schmidt orthogonalize tangent with normal.

		nDotT = pVertex0->normal[0]*pVertex0->tangent[0] +
			pVertex0->normal[1]*pVertex0->tangent[1] +
			pVertex0->normal[2]*pVertex0->tangent[2];

		pVertex0->tangent[0] -= pVertex0->normal[0]*nDotT;
		pVertex0->tangent[1] -= pVertex0->normal[1]*nDotT;
		pVertex0->tangent[2] -= pVertex0->normal[2]*nDotT;

		// Normalize the tangent.

		invlength = 1.0f / sqrtf(pVertex0->tangent[0]*pVertex0->tangent[0] +
				      pVertex0->tangent[1]*pVertex0->tangent[1] +
				      pVertex0->tangent[2]*pVertex0->tangent[2]);

		pVertex0->tangent[0] *= invlength;
		pVertex0->tangent[1] *= invlength;
		pVertex0->tangent[2] *= invlength;

		// Calculate the handedness of the local tangent space.
		// The bitangent vector is the cross product between the triangle face
		// normal vector and the calculated tangent vector. The resulting
		// bitangent vector should be the same as the bitangent vector
		// calculated from the set of linear equations above. If they point in
		// different directions then we need to invert the cross product
		// calculated bitangent vector. We store this scalar multiplier in the
		// tangent vector's 'w' component so that the correct bitangent vector
		// can be generated in the normal mapping shader's vertex shader.
		//
		// Normal maps have a left handed coordinate system with the origin
		// located at the top left of the normal map texture. The x coordinates
		// run horizontally from left to right. The y coordinates run
		// vertically from top to bottom. The z coordinates run out of the
		// normal map texture towards the viewer. Our handedness calculations
		// must take this fact into account as well so that the normal mapping
		// shader's vertex shader will generate the correct bitangent vectors.
		// Some normal map authoring tools such as Crazybump
		// (http://www.crazybump.com/) includes options to allow you to control
		// the orientation of the normal map normal's y-axis.

		bitangent[0] = (pVertex0->normal[1]*pVertex0->tangent[2]) -
			       (pVertex0->normal[2]*pVertex0->tangent[1]);
		bitangent[1] = (pVertex0->normal[2]*pVertex0->tangent[0]) -
			       (pVertex0->normal[0]*pVertex0->tangent[2]);
		bitangent[2] = (pVertex0->normal[0]*pVertex0->tangent[1]) -
			       (pVertex0->normal[1]*pVertex0->tangent[0]);

		bDotB = bitangent[0]*pVertex0->bitangent[0] +
			bitangent[1]*pVertex0->bitangent[1] +
			bitangent[2]*pVertex0->bitangent[2];

		pVertex0->tangent[3] = (bDotB < 0.0f) ? 1.0f : -1.0f;

		pVertex0->bitangent[0] = bitangent[0];
		pVertex0->bitangent[1] = bitangent[1];
		pVertex0->bitangent[2] = bitangent[2];
	}
}

void StelOBJ::generateAABB()
{
	//calculate AABB and centroid for each object
	Vec3d accCentroid(0.);
	m_bbox.reset();
	for(int i =0;i<m_objects.size();++i)
	{
		Vec3d centr(0.);
		Object& o = m_objects[i];

		o.postprocess(*this,centr);
		m_bbox.expand(o.boundingbox);
		accCentroid+=centr;
	}

	m_centroid = (accCentroid / m_objects.size()).toVec3f();
}

void StelOBJ::performPostProcessing(bool genNormals)
{
	QElapsedTimer timer;
	timer.start();

	//if no normals have been read at all, or if normals are broken in the OBJ file and flagged as such in the scenery3d.ini, (re-)generate them (we do not support smoothing groups at the time, so this is quite simple)
	if(genNormals)
	{
		generateNormals();
		qCDebug(stelOBJ)<<"Normals calculated in"<<timer.restart()<<"ms";
	}

	//generate tangent data
	generateTangents();
	qCDebug(stelOBJ())<<"Tangents calculated in"<<timer.restart()<<"ms";

	generateAABB();
	qCDebug(stelOBJ)<<"AABBs/Centroids calculated in"<<timer.elapsed()<<"ms";
	qCDebug(stelOBJ)<<"Centroid is at "<<m_centroid;
}

StelOBJ::ShortIndexList StelOBJ::getShortIndexList() const
{
	QElapsedTimer timer;
	timer.start();

	ShortIndexList ret;
	if(!canUseShortIndices())
	{
		qCWarning(stelOBJ)<<"Cannot use short indices for OBJ data, it has"<<m_vertices.size()<<"vertices";
		return ret;
	}

	ret.reserve(m_indices.size());
	for(int i =0;i<m_indices.size();++i)
	{
		ret.append(static_cast<unsigned short>(m_indices.at(i)));
	}

	qCDebug(stelOBJ)<<"Indices converted to short in"<<timer.elapsed()<<"ms";
	return ret;
}

void StelOBJ::scale(double factor)
{
	QElapsedTimer timer;
	timer.start();

	for(int i = 0;i<m_vertices.size();++i)
	{
		GLfloat* dat = m_vertices[i].position;
		dat[0] *= static_cast<GLfloat>(factor);
		dat[1] *= static_cast<GLfloat>(factor);
		dat[2] *= static_cast<GLfloat>(factor);
	}

	//AABBs must be recalculated
	generateAABB();
	qCDebug(stelOBJ)<<"Scaling done in"<<timer.elapsed()<<"ms";
}

void StelOBJ::transform(const QMatrix4x4 &mat, bool onlyPosition)
{
	//matrix for normals/tangents
	QMatrix3x3 normalMat = mat.normalMatrix();

	//Transform all vertices and normals by mat
	for(int i=0; i<m_vertices.size(); ++i)
	{
		Vertex& pVertex = m_vertices[i];

		QVector3D tf = mat.map(QVector3D(pVertex.position[0], pVertex.position[1], pVertex.position[2]));
		std::copy(&tf[0],&tf[0]+3,pVertex.position);

		if(!onlyPosition)
		{
			tf = normalMat * QVector3D(pVertex.normal[0], pVertex.normal[1], pVertex.normal[2]);
			pVertex.normal[0] = tf.x();
			pVertex.normal[1] = tf.y();
			pVertex.normal[2] = tf.z();

			tf = normalMat * QVector3D(pVertex.tangent[0], pVertex.tangent[1], pVertex.tangent[2]);
			pVertex.tangent[0] = tf.x();
			pVertex.tangent[1] = tf.y();
			pVertex.tangent[2] = tf.z();

			tf = normalMat * QVector3D(pVertex.bitangent[0], pVertex.bitangent[1], pVertex.bitangent[2]);
			pVertex.bitangent[0] = tf.x();
			pVertex.bitangent[1] = tf.y();
			pVertex.bitangent[2] = tf.z();
		}
	}

	//Update bounding box in case it changed
	generateAABB();
}

void StelOBJ::splitVertexData(V3Vec *position,
			      V2Vec *texCoord,
			      V3Vec *normal,
			      V3Vec *tangent,
			      V3Vec *bitangent) const
{
	QElapsedTimer timer;
	timer.start();

	const int size = m_vertices.size();
	//resize arrays
	if(position)
		position->resize(size);
	if(texCoord)
		texCoord->resize(size);
	if(normal)
		normal->resize(size);
	if(tangent)
		tangent->resize(size);
	if(bitangent)
		bitangent->resize(size);

	for(int i = 0;i<size;++i)
	{
		const Vertex& vtx = m_vertices.at(i);
		if(position)
			(*position)[i] = Vec3f(vtx.position);
		if(texCoord)
			(*texCoord)[i] = Vec2f(vtx.texCoord);
		if(normal)
			(*normal)[i] = Vec3f(vtx.normal);
		if(tangent)
			(*tangent)[i] = Vec3f(vtx.tangent);
		if(bitangent)
			(*bitangent)[i] = Vec3f(vtx.bitangent);
	}		

	qCDebug(stelOBJ)<<"Vertex data split in "<<timer.elapsed()<<"ms";
}

void StelOBJ::clearVertexData()
{
	m_vertices.clear();
}
