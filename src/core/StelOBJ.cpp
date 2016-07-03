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
#include <QFileInfo>
#include <QElapsedTimer>
#include <QFile>
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
	m_vertices.clear();
	m_indices.clear();
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
		qCCritical(stelOBJ)<<"Could not open file "<<filename<<file.errorString();
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

bool StelOBJ::parseVertPos(const QVector<QStringRef>& params, V3Vec& posList) const
{
	if(params.size()<4 || params.size()>5)
	{
		qCCritical(stelOBJ)<<"Invalid vertex position specification"<<params;
		return false;
	}
	//we ignore the optional W coordinate
	if(params.size()==5)
		qCWarning(stelOBJ)<<"W coordinate unsupported in vertex position specification"<<params;

	Vec3f v;
	bool ok = false;
	v[0] = params.at(1).toFloat(&ok);
	if(ok)
	{
		v[1] = params.at(2).toFloat(&ok);
		if(ok)
		{
			v[2] = params.at(3).toFloat(&ok);
			posList.append(v);
			return true;
		}
	}

	qCCritical(stelOBJ)<<"Cannot parse vertex position specification"<<params;
	return false;
}

bool StelOBJ::parseVertNorm(ParseParams params, V3Vec& normList) const
{
	if(params.size()!=4)
	{
		qCCritical(stelOBJ)<<"Invalid vertex normal specification"<<params;
		return false;
	}

	Vec3f v;
	bool ok = false;
	v[0] = params.at(1).toFloat(&ok);
	if(ok)
	{
		v[1] = params.at(2).toFloat(&ok);
		if(ok)
		{
			v[2] = params.at(3).toFloat(&ok);
			normList.append(v);
			return true;
		}
	}

	qCCritical(stelOBJ)<<"Cannot parse vertex normal specification"<<params;
	return false;
}

bool StelOBJ::parseVertTex(ParseParams params, V2Vec& texList) const
{
	if(params.size()<3||params.size()>4)
	{
		qCCritical(stelOBJ)<<"Invalid vertex texture coordinate specification"<<params;
		return false;
	}

	Vec2f v;
	bool ok = false;
	v[0] = params.at(1).toFloat(&ok);
	if(ok)
	{
		v[1] = params.at(2).toFloat(&ok);
		texList.append(v);
		return true;
	}

	qCCritical(stelOBJ)<<"Cannot parse vertex texture coordinate specification"<<params;
	return false;
}

bool StelOBJ::parseFace(ParseParams params, const V3Vec& posList, const V3Vec& normList, const V2Vec& texList,
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
		Vertex v = {};
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

	//vertex data has been loaded, create the faces
	//we use triangle-fan triangulation
	for(int i=2;i<vtxAmount;++i)
	{
		//the first one is always the same
		m_indices.append(vIdx[0]);
		m_indices.append(vIdx[i-1]);
		m_indices.append(vIdx[i]);
	}

	return true;
}

bool StelOBJ::load(QIODevice& device, const QString &basePath)
{
	clear();

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

			//macro to test out different ways of comparison and their performance
			#define CMD_CMP(a) (cmd==QLatin1String(a))

			if(CMD_CMP("f"))
			{
				ok = parseFace(splits,posList,normalList,texList,vertCache);
			}
			else if(CMD_CMP("v"))
			{
				ok = parseVertPos(splits,posList);
			}
			else if(CMD_CMP("vt"))
			{
				ok = parseVertTex(splits,texList);
			}
			else if(CMD_CMP("vn"))
			{
				ok = parseVertNorm(splits,normalList);
			}
			else if(CMD_CMP("usemtl"))
			{
				//load material from cache
			}
			else if(CMD_CMP("mtllib"))
			{
				//load external material file
			}
			else if(CMD_CMP("o"))
			{
				//create new object
			}
			else if(CMD_CMP("g"))
			{
				//create new group
			}
			else if(!CMD_CMP("#"))
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
	qCDebug(stelOBJ, "Parsed %d positions, %d normals, %d texture coordinates",
		posList.size(), normalList.size(), texList.size());
	qCDebug(stelOBJ, "Created %d vertices and %d faces", m_vertices.size(), getFaceCount());


	device.close();
	return true;
}
