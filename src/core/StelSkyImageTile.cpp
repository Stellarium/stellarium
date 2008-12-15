/*
 * Copyright (C) 2008 Fabien Chereau
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "StelSkyImageTile.hpp"
#include "StelApp.hpp"
#include "StelFileMgr.hpp"
#include "StelUtils.hpp"
#include "StelTexture.hpp"
#include "StelProjector.hpp"
#include "StelToneReproducer.hpp"
#include "StelCore.hpp"
#include "StelTextureMgr.hpp"
#include "StelSkyDrawer.hpp"
#include "StelPainter.hpp"

#include <QDebug>

#ifdef DEBUG_STELSKYIMAGE_TILE
 #include "StelFont.hpp"
#include "StelFontMgr.hpp"
#include "StelLocaleMgr.hpp"
 StelFont* StelSkyImageTile::debugFont = NULL;
#endif

void StelSkyImageTile::initCtor()
{
	minResolution = -1;
	luminance = -1;
	alphaBlend = false;
	noTexture = false;
	texFader = NULL;
}

// Constructor
StelSkyImageTile::StelSkyImageTile(const QString& url, StelSkyImageTile* parent) : MultiLevelJsonBase(parent)
{
	initCtor();
	if (parent!=NULL)
	{
		luminance = parent->luminance;
		alphaBlend = parent->alphaBlend;
	}
	initFromUrl(url);
}

// Constructor from a map used for JSON files with more than 1 level
StelSkyImageTile::StelSkyImageTile(const QVariantMap& map, StelSkyImageTile* parent) : MultiLevelJsonBase(parent)
{
	initCtor();
	if (parent!=NULL)
	{
		luminance = parent->luminance;
		alphaBlend = parent->alphaBlend;
	}
	initFromQVariantMap(map);
}
	
// Destructor
StelSkyImageTile::~StelSkyImageTile()
{
}
	
void StelSkyImageTile::draw(StelCore* core, const StelPainter& sPainter)
{
	const StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);
	
	const float limitLuminance = core->getSkyDrawer()->getLimitLuminance();
	QMultiMap<double, StelSkyImageTile*> result;
	getTilesToDraw(result, core, prj->getViewportConvexPolygon(0, 0), limitLuminance, true);
	
	int numToBeLoaded=0;
	foreach (StelSkyImageTile* t, result)
		if (t->isReadyToDisplay()==false)
			++numToBeLoaded;
	updatePercent(result.size(), numToBeLoaded);

	// Draw in the good order
	glEnable(GL_TEXTURE_2D);
	glBlendFunc(GL_ONE, GL_ONE);
	QMap<double, StelSkyImageTile*>::Iterator i = result.end();
	while (i!=result.begin())
	{
		--i;
		i.value()->drawTile(core, sPainter);
	}
	
	deleteUnusedSubTiles();
}
	
// Return the list of tiles which should be drawn.
void StelSkyImageTile::getTilesToDraw(QMultiMap<double, StelSkyImageTile*>& result, StelCore* core, const StelGeom::ConvexPolygon& viewPortPoly, float limitLuminance, bool recheckIntersect)
{
	
#ifndef NEDBUG
	// This method should be called only if the parent min resolution was reached
	const StelSkyImageTile* parent = qobject_cast<StelSkyImageTile*>(QObject::parent());
	if (parent!=NULL)
	{
		const double degPerPixel = 1./core->getProjection(StelCore::FrameJ2000)->getPixelPerRadAtCenter()*180./M_PI;
		Q_ASSERT(degPerPixel<parent->minResolution);
	}
#endif
		
	// An error occured during loading
	if (errorOccured)
		return;
	
	// The JSON file is currently being downloaded
	if (downloading)
		return;
	
	if (luminance>0 && luminance<limitLuminance)
	{
		// Schedule a deletion
		scheduleDeletion();
		return;
	}
	
	// Check that we are in the screen
	bool fullInScreen = true;
	bool intersectScreen = false;
	if (recheckIntersect)
	{
		if (skyConvexPolygons.isEmpty())
		{
			// If no polygon is defined, we assume that the tile covers the whole sky
			fullInScreen=false;
			intersectScreen=true;
		}
		else
		{
			foreach (const StelGeom::ConvexPolygon poly, skyConvexPolygons)
			{
				if (contains(viewPortPoly, poly))
				{
					intersectScreen = true;
				}
				else
				{
					fullInScreen = false;
					if (intersect(viewPortPoly, poly))
						intersectScreen = true;
				}
			}
		}
	}
	// The tile is outside screen
	if (fullInScreen==false && intersectScreen==false)
	{
		// Schedule a deletion
		scheduleDeletion();
		return;
	}
	
	// The tile is in screen, and it is a precondition that its resolution is higher than the limit
	// make sure that it's not going to be deleted
	cancelDeletion();
	
	if (noTexture==false)
	{
		if (!tex)
		{
			// The tile has an associated texture, but it is not yet loaded: load it now
			StelTextureMgr& texMgr=StelApp::getInstance().getTextureManager();
			texMgr.setDefaultParams();
 			texMgr.setMipmapsMode(true);
			texMgr.setMinFilter(GL_LINEAR);
 			texMgr.setMagFilter(GL_LINEAR);
 			texMgr.setWrapMode(GL_CLAMP_TO_EDGE);
			tex = texMgr.createTextureThread(absoluteImageURI);
			if (!tex)
			{
				qWarning() << "WARNING : Can't create tile: " << absoluteImageURI;
				errorOccured = true;
				return;
			}
		}
		
		// The tile is in screen and has a texture: every test passed :) The tile will be displayed
		result.insert(minResolution, this);
	}
	
	// Check if we reach the resolution limit
	const double degPerPixel = 1./core->getProjection(StelCore::FrameJ2000)->getPixelPerRadAtCenter()*180./M_PI;
	if (degPerPixel < minResolution)
	{
		if (subTiles.isEmpty() && !subTilesUrls.isEmpty())
		{
			// Load the sub tiles because we reached the maximum resolution and they are not yet loaded
			foreach (QVariant s, subTilesUrls)
			{
				StelSkyImageTile* nt;
				if (s.type()==QVariant::Map)
					nt = new StelSkyImageTile(s.toMap(), this);
				else
				{
					Q_ASSERT(s.type()==QVariant::String);
					nt = new StelSkyImageTile(s.toString(), this);
				}
				subTiles.append(nt);
			}
		}
		// Try to add the subtiles
		foreach (MultiLevelJsonBase* tile, subTiles)
		{
			qobject_cast<StelSkyImageTile*>(tile)->getTilesToDraw(result, core, viewPortPoly, limitLuminance, !fullInScreen);
		}
	}
	else
	{
		foreach (MultiLevelJsonBase* tile, subTiles)
		{
			tile->scheduleDeletion();
		}
	}
}

// Draw the image on the screen.
// Assume GL_TEXTURE_2D is enabled
bool StelSkyImageTile::drawTile(StelCore* core, const StelPainter& sPainter)
{	
	if (!tex->bind())
	{
		return false;
	}
	
	if (!texFader)
	{
		texFader = new QTimeLine(1000, this);
		texFader->start();
	}
	
	const StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);
	const float factorX = tex->getCoordinates()[2][0];
	const float factorY = tex->getCoordinates()[2][1];

	// Draw the real texture for this image
	const float ad_lum = (luminance>0) ? core->getToneReproducer()->adaptLuminanceScaled(luminance) : 1.f;
	if (alphaBlend==true || texFader->state()==QTimeLine::Running)
	{
		if (!alphaBlend)
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Normal transparency mode
		glEnable(GL_BLEND);
		glColor4f(ad_lum,ad_lum,ad_lum, texFader->currentValue());
	}
	else
	{
		glDisable(GL_BLEND);
		glColor3f(ad_lum,ad_lum,ad_lum);
	}
	
	for (int p=0;p<skyConvexPolygons.size();++p)
	{
		const StelGeom::Polygon& poly = skyConvexPolygons.at(p).asPolygon();
		const QList<Vec2f>& texCoords = textureCoords.at(p);
				
		Q_ASSERT((int)poly.size()==texCoords.size());
						
		Vec3d win;
		const int N=poly.size()-1;
		int idx=N;
		int diff = 0;
		// Using TRIANGLE STRIP requires to use the following vertex order N-0,0,N-1,1,N-2,2 etc..
		glBegin(GL_TRIANGLE_STRIP);
		for (int i=0;i<=N;++i)
		{
			idx = (diff==0 ? N-i/2 : i/2);
			++diff;
			if (diff>1) diff=0;
					
			glTexCoord2d(texCoords[idx][0]*factorX, texCoords[idx][1]*factorY);
			prj->project(poly[idx],win);
			glVertex3dv(win);
		}
		glEnd();
	}
#ifdef DEBUG_STELSKYIMAGE_TILE
	if (debugFont==NULL)
	{
		debugFont = &StelApp::getInstance().getFontManager().getStandardFont(StelApp::getInstance().getLocaleMgr().getSkyLanguage(), 12);
	}
	glColor3f(1.0,0.5,0.5);
	foreach (const StelGeom::ConvexPolygon& poly, skyConvexPolygons)
	{
		Vec3d win;
		Vec3d bary = poly.getBarycenter();
		prj->project(bary,win);
		sPainter.drawText(debugFont, win[0], win[1], getAbsoluteImageURI());
		
		glDisable(GL_TEXTURE_2D);
		sPainter.drawPolygon(poly);
		glEnable(GL_TEXTURE_2D);
	}
#endif
	if (!alphaBlend)
		glBlendFunc(GL_ONE, GL_ONE); // Revert
	
	return true;
}

// Return true if the tile is fully loaded and can be displayed
bool StelSkyImageTile::isReadyToDisplay() const
{
	return tex && tex->canBind();
}

// Load the tile from a valid QVariantMap
void StelSkyImageTile::loadFromQVariantMap(const QVariantMap& map)
{
	if (map.contains("imageCredits"))
	{
		QVariantMap dsCredits = map.value("imageCredits").toMap();
		dataSetCredits.shortCredits = dsCredits.value("short").toString();
		dataSetCredits.fullCredits = dsCredits.value("full").toString();
		dataSetCredits.infoURL = dsCredits.value("infoUrl").toString();
	}
	if (map.contains("serverCredits"))
	{
		QVariantMap sCredits = map.value("serverCredits").toMap();
		serverCredits.shortCredits = sCredits.value("short").toString();
		serverCredits.fullCredits = sCredits.value("full").toString();
		serverCredits.infoURL = sCredits.value("infoUrl").toString();
	}
	
	shortName = map.value("shortName").toString();
	bool ok=false;
	minResolution = map.value("minResolution").toDouble(&ok);
	if (!ok)
		throw std::runtime_error("minResolution expect a double value");
	
	if (map.contains("luminance"))
	{
		luminance = map.value("luminance").toDouble(&ok);
		if (!ok)
			throw std::runtime_error("luminance expect a float value");
		qWarning() << "luminance in preview JSON files is deprecated. Replace with maxBrightness.";
	}
	
	if (map.contains("maxBrightness"))
	{
		luminance = map.value("maxBrightness").toDouble(&ok);
		if (!ok)
			throw std::runtime_error("maxBrightness expect a float value");
		luminance = StelApp::getInstance().getCore()->getSkyDrawer()->surfacebrightnessToLuminance(luminance);
	}
	
	if (map.contains("alphaBlend"))
	{
		alphaBlend = map.value("alphaBlend").toBool();
	}
	
	// Load the convex polygons (if any)
	QVariantList polyList = map.value("skyConvexPolygons").toList();
	if (polyList.empty())
		polyList = map.value("worldCoords").toList();
	else
		qWarning() << "skyConvexPolygons in preview JSON files is deprecated. Replace with worldCoords.";
	foreach (const QVariant& polyRaDec, polyList)
	{
		QList<Vec3d> vertices;
		foreach (QVariant vRaDec, polyRaDec.toList())
		{
			const QVariantList vl = vRaDec.toList();
			Vec3d v;
			StelUtils::spheToRect(vl.at(0).toDouble(&ok)*M_PI/180., vl.at(1).toDouble(&ok)*M_PI/180., v);
			if (!ok)
				throw std::runtime_error("wrong Ra and Dec, expect a double value");
			vertices.append(v);
		}
		Q_ASSERT(vertices.size()==4);
		skyConvexPolygons.append(StelGeom::ConvexPolygon(vertices[0], vertices[1], vertices[2], vertices[3]));
	}
	
	// Load the matching textures positions (if any)
	polyList = map.value("textureCoords").toList();
	foreach (const QVariant& polyXY, polyList)
	{
		QList<Vec2f> vertices;
		foreach (QVariant vXY, polyXY.toList())
		{
			const QVariantList vl = vXY.toList();
			vertices.append(Vec2f(vl.at(0).toDouble(&ok), vl.at(1).toDouble(&ok)));
			if (!ok)
				throw std::runtime_error("wrong X and Y, expect a double value");
		}
		Q_ASSERT(vertices.size()==4);
		textureCoords.append(vertices);
	}
	
	if (map.contains("imageUrl"))
	{
		QString imageUrl = map.value("imageUrl").toString();
		if (baseUrl.startsWith("http://"))
		{
			absoluteImageURI = baseUrl+imageUrl;
		}
		else
		{
			try
			{
				absoluteImageURI = StelApp::getInstance().getFileMgr().findFile(baseUrl+imageUrl);
			}
			catch (std::runtime_error& er)
			{
				// Maybe the user meant a file in stellarium local files
				absoluteImageURI = imageUrl;
			}
		}
		if (skyConvexPolygons.size()!=textureCoords.size())
			throw std::runtime_error("the number of convex polygons does not match the number of texture space polygon");
	}
	else
		noTexture = true;
	
	// This is a list of URLs to the child tiles or a list of already loaded map containing child information
	// (in this later case, the StelSkyImageTile objects will be created later)
	subTilesUrls = map.value("subTiles").toList();
// 	if (subTilesUrls.size()>10)
// 	{
// 		qWarning() << "Large tiles number for " << shortName << ": " << subTilesUrls.size();
// 	}
}

// Convert the image informations to a map following the JSON structure.
QVariantMap StelSkyImageTile::toQVariantMap() const
{
	QVariantMap res;
	
	// Image credits
	QVariantMap imCredits;
	if (!dataSetCredits.shortCredits.isEmpty())
		imCredits["short"]=dataSetCredits.shortCredits;
	if (!dataSetCredits.fullCredits.isEmpty())
		imCredits["full"]=dataSetCredits.fullCredits;
	if (!dataSetCredits.infoURL.isEmpty())
		imCredits["infoUrl"]=dataSetCredits.infoURL;
	if (!imCredits.empty())
		res["imageCredits"]=imCredits;
	
	// Server credits
	QVariantMap serCredits;
	if (!serverCredits.shortCredits.isEmpty())
		imCredits["short"]=serverCredits.shortCredits;
	if (!serverCredits.fullCredits.isEmpty())
		imCredits["full"]=serverCredits.fullCredits;
	if (!serverCredits.infoURL.isEmpty())
		imCredits["infoUrl"]=serverCredits.infoURL;
	if (!serCredits.empty())
		res["serverCredits"]=serCredits;
	
	// Misc
	if (!shortName.isEmpty())
		res["shortName"] = shortName;	
	if (minResolution>0)
		 res["minResolution"]=minResolution;
	if (luminance>0)
		res["maxBrightness"]=StelApp::getInstance().getCore()->getSkyDrawer()->luminanceToSurfacebrightness(luminance);
	if (alphaBlend)
		res["alphaBlend"]=true;
	if (noTexture==false)
		res["imageUrl"]=absoluteImageURI;
	
	// Polygons
	if (!skyConvexPolygons.isEmpty())
	{
		QVariantList polygsL;
		foreach (const StelGeom::ConvexPolygon& poly, skyConvexPolygons)
		{
			QVariantList polyL;
			for (size_t i=0;i<poly.asPolygon().size();++i)
			{
				double ra, dec;
				StelUtils::rectToSphe(&ra, &dec, poly[i]);
				QVariantList vL;
				vL.append(ra);
				vL.append(dec);
				polyL.append(vL);
			}
			polygsL.append(polyL);
		}
		res["worldCoords"]=polygsL;
	}
	
	// textures positions
	if (!textureCoords.empty())
	{
		QVariantList polygsL;
		foreach (const QList<Vec2f>& poly, textureCoords)
		{
			QVariantList polyL;
			foreach (Vec2f v, poly)
			{
				QVariantList vL;
				vL.append(v[0]);
				vL.append(v[1]);
				polyL.append(vL);
			}
			polygsL.append(polyL);
		}
		res["textureCoords"]=polygsL;
	}
	
	if (!subTilesUrls.empty())
	{
		res["subTiles"] = subTilesUrls;
	}
	
	return res;
}
