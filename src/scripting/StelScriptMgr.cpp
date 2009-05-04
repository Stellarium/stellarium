/*
 * Stellarium
 * Copyright (C) 2009 Matthew Gates
 * Copyright (C) 2007 Fabien Chereau
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

#include "fixx11h.h"

#include "StelScriptMgr.hpp"
#include "StelMainScriptAPI.hpp"

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelFileMgr.hpp"
#include "StelGui.hpp"
#include "StelModuleMgr.hpp"
#include "StelMovementMgr.hpp"
#include "StelNavigator.hpp"
#include "StelSkyDrawer.hpp"
#include "StelSkyImageMgr.hpp"

#include <QAction>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegExp>
#include <QSet>
#include <QStringList>
#include <QTemporaryFile>
#include <QVariant>

#include <cmath>

Q_DECLARE_METATYPE(Vec3f);

class StelScriptThread : public QThread
{
	public:
		StelScriptThread(const QString& ascriptCode, QScriptEngine* aengine, QString fileName) : scriptCode(ascriptCode), engine(aengine), fname(fileName) {;}
		QString getFileName() {return fname;}
		
	protected:
		void run()
		{
			// seed the QT PRNG
			qsrand(QDateTime::currentDateTime().toTime_t());

			// For startup scripts, the gui object might not 
			// have completed init when we run. Wait for that.
			StelGui* gui = GETSTELMODULE(StelGui);
			while(!gui)
			{
				msleep(200);
				gui = GETSTELMODULE(StelGui);
			}
			while(!gui->initComplete())
				msleep(200);

			engine->evaluate(scriptCode);
		}

	private:
		QString scriptCode;
		QScriptEngine* engine;
		QString fname;
};


QScriptValue vec3fToScriptValue(QScriptEngine *engine, const Vec3f& c)
{
	QScriptValue obj = engine->newObject();
	obj.setProperty("r", QScriptValue(engine, c[0]));
	obj.setProperty("g", QScriptValue(engine, c[1]));
	obj.setProperty("b", QScriptValue(engine, c[2]));
	return obj;
}

void vec3fFromScriptValue(const QScriptValue& obj, Vec3f& c)
{
	c[0] = obj.property("r").toNumber();
	c[1] = obj.property("g").toNumber();
	c[2] = obj.property("b").toNumber();
}

QScriptValue createVec3f(QScriptContext* context, QScriptEngine *engine)
{
	Vec3f c;
	c[0] = context->argument(0).toNumber();
	c[1] = context->argument(1).toNumber();
	c[2] = context->argument(2).toNumber();
	return vec3fToScriptValue(engine, c);
}

StelScriptMgr::StelScriptMgr(QObject *parent) 
	: QObject(parent), 
	  thread(NULL)
{
	// Allow Vec3f managment in scripts
	qScriptRegisterMetaType(&engine, vec3fToScriptValue, vec3fFromScriptValue);
	// Constructor
	QScriptValue ctor = engine.newFunction(createVec3f);
	engine.globalObject().setProperty("Vec3f", ctor);
	
	// Add the core object to access methods related to core
	mainAPI = new StelMainScriptAPI(this);
	QScriptValue objectValue = engine.newQObject(mainAPI);
	engine.globalObject().setProperty("core", objectValue);
	
	// Add all the StelModules into the script engine
	StelModuleMgr* mmgr = &StelApp::getInstance().getModuleMgr();
	foreach (StelModule* m, mmgr->getAllModules())
	{
		objectValue = engine.newQObject(m);
		engine.globalObject().setProperty(m->objectName(), objectValue);
	}

	// Add other classes which we want to be directly accessible from scripts
	if(StelSkyImageMgr* smgr = GETSTELMODULE(StelSkyImageMgr))
		objectValue = engine.newQObject(smgr);

	// For accessing star scale, twinkle etc.
	objectValue = engine.newQObject(StelApp::getInstance().getCore()->getSkyDrawer());
	engine.globalObject().setProperty("StelSkyDrawer", objectValue);
}


StelScriptMgr::~StelScriptMgr()
{
}

QStringList StelScriptMgr::getScriptList(void)
{
	QStringList scriptFiles;
	try
	{
		StelFileMgr& fileMan(StelApp::getInstance().getFileMgr());
		QSet<QString> files = fileMan.listContents("scripts",StelFileMgr::File, true);
		foreach(QString f, files)
		{
#ifdef ENABLE_STRATOSCRIPT_COMPAT
			QRegExp fileRE("^.*\\.(ssc|sts)$");
#else // ENABLE_STRATOSCRIPT_COMPAT
			QRegExp fileRE("^.*\\.ssc$");
#endif // ENABLE_STRATOSCRIPT_COMPAT
			if (fileRE.exactMatch(f))
				scriptFiles << f;
		}
	}
	catch (std::runtime_error& e)
	{
		QString msg = QString("WARNING: could not list scripts: %1").arg(e.what());
		qWarning() << msg;
		emit(scriptDebug(msg));
	}
	return scriptFiles;
}

bool StelScriptMgr::scriptIsRunning(void)
{
	return (thread != NULL);
}

QString StelScriptMgr::runningScriptId(void)
{
	if (thread)
		return thread->getFileName();
	else
		return QString();
}

const QString StelScriptMgr::getHeaderSingleLineCommentText(const QString& s, const QString& id, const QString& notFoundText)
{
	try
	{
		QFile file(StelApp::getInstance().getFileMgr().findFile("scripts/" + s, StelFileMgr::File));
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			QString msg = QString("WARNING: script file %1 could not be opened for reading").arg(s);
			emit(scriptDebug(msg));
			qWarning() << msg;
			return QString();
		}

		QRegExp nameExp("^\\s*//\\s*" + id + ":\\s*(.+)$");
		while (!file.atEnd()) {
			QString line(file.readLine());
			if (nameExp.exactMatch(line))
			{
				file.close();	
				return nameExp.capturedTexts().at(1);
			}
		}
		file.close();
		return notFoundText;
	}
	catch(std::runtime_error& e)
	{
		QString msg = QString("WARNING: script file %1 could not be found: %2").arg(s).arg(e.what());
		emit(scriptDebug(msg));
		qWarning() << msg;
		return QString();
	}
}

const QString StelScriptMgr::getName(const QString& s)
{
	return getHeaderSingleLineCommentText(s, "Name", s);
}

const QString StelScriptMgr::getAuthor(const QString& s)
{
	return getHeaderSingleLineCommentText(s, "Author");
}

const QString StelScriptMgr::getLicense(const QString& s)
{
	return getHeaderSingleLineCommentText(s, "License", "");
}

const QString StelScriptMgr::getDescription(const QString& s)
{
	try
	{
		QFile file(StelApp::getInstance().getFileMgr().findFile("scripts/" + s, StelFileMgr::File));
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			QString msg = QString("WARNING: script file %1 could not be opened for reading").arg(s);
			emit(scriptDebug(msg));
			qWarning() << msg;
			return QString();
		}

		QString desc = "";
		bool inDesc = false;
		QRegExp descExp("^\\s*//\\s*Description:\\s*([^\\s].+)\\s*$");
		QRegExp descNewlineExp("^\\s*//\\s*$");
		QRegExp descContExp("^\\s*//\\s*([^\\s].*)\\s*$");
		while (!file.atEnd()) {
			QString line(file.readLine());
			
			if (!inDesc && descExp.exactMatch(line))
			{
				inDesc = true;
				desc = descExp.capturedTexts().at(1) + " ";
				desc.replace("\n","");
			}
			else if (inDesc)
			{
				QString d("");
				if (descNewlineExp.exactMatch(line))
					d = "\n";
				else if (descContExp.exactMatch(line))
				{
					d = descContExp.capturedTexts().at(1) + " ";
					d.replace("\n","");
				}
				else
				{
					file.close();	
					return desc;
				}
				desc += d;
			}
		}
		file.close();
		return desc;
	}
	catch(std::runtime_error& e)
	{
		QString msg = QString("WARNING: script file %1 could not be found: %2").arg(s).arg(e.what());
		emit(scriptDebug(msg));
		qWarning() << msg;
		return QString();
	}
}

// Run the script located at the given location
bool StelScriptMgr::runScript(const QString& fileName, const QString& includePath)
{
	if (thread!=NULL)
	{
		QString msg = QString("ERROR: there is already a script running, please wait that it's over.");
		emit(scriptDebug(msg));
		qWarning() << msg;
		return false;
	}
	QString absPath;
	QString scriptDir;
	try
	{
		if (QFileInfo(fileName).isAbsolute())
			absPath = fileName;
		else
			absPath = StelApp::getInstance().getFileMgr().findFile("scripts/" + fileName);

		scriptDir = QFileInfo(absPath).dir().path();
	}
	catch (std::runtime_error& e)
	{
		QString msg = QString("WARNING: could not find script file %1: %2").arg(fileName).arg(e.what());
		emit(scriptDebug(msg));
		qWarning() << msg;
		return false;
	}
	// pre-process the script into a temporary file
	QTemporaryFile tmpFile;
	bool ok = false;
	if (!tmpFile.open())
	{
		QString msg = QString("WARNING: cannot create temporary file for script pre-processing");
		emit(scriptDebug(msg));
		qWarning() << msg;
		return false;
	}
	QFile fic(absPath);
	if (!fic.open(QIODevice::ReadOnly))
	{
		QString msg = QString("WARNING: cannot open script: %1").arg(fileName);
		emit(scriptDebug(msg));
		qWarning() << msg;
		tmpFile.close();
		return false;
	}

	if (includePath!="" && !includePath.isEmpty())
		scriptDir = includePath;

	if (fileName.right(4) == ".ssc")
		ok = preprocessScript(fic, tmpFile, scriptDir);
#ifdef ENABLE_STRATOSCRIPT_COMPAT
	else if (fileName.right(4) == ".sts")
		ok = preprocessStratoScript(fic, tmpFile, scriptDir);
#endif

	fic.close();

	if (ok==false)
	{
		tmpFile.close();
		return false;
	}

	tmpFile.seek(0);
	thread = new StelScriptThread(QTextStream(&tmpFile).readAll(), &engine, fileName);
	tmpFile.close();
	
	GETSTELMODULE(StelGui)->setScriptKeys(true);
	connect(thread, SIGNAL(finished()), this, SLOT(scriptEnded()));
	thread->start();
	emit(scriptRunning());
	return true;
}

bool StelScriptMgr::stopScript(void)
{
	if (thread)
	{
		QString msg = QString("INFO: asking running script to exit");
		emit(scriptDebug(msg));
		qDebug() << msg;
		thread->terminate();
		return true;
	}
	else
	{
		QString msg = QString("WARNING: no script is running");
		emit(scriptDebug(msg));
		qWarning() << msg;
		return false;
	}
}

void StelScriptMgr::setScriptRate(double r)
{
	// pre-calculate the new time rate in an effort to prevent there being much latency
	// between setting the script rate and the time rate.
	double factor = r / mainAPI->getScriptSleeper().getRate();
	StelNavigator* nav = StelApp::getInstance().getCore()->getNavigator();
	double newTimeRate = nav->getTimeRate() * factor;

	mainAPI->getScriptSleeper().setRate(r);
	if (scriptIsRunning()) nav->setTimeRate(newTimeRate);
}

double StelScriptMgr::getScriptRate(void)
{
	return mainAPI->getScriptSleeper().getRate();
}

void StelScriptMgr::debug(const QString& msg)
{
	emit(scriptDebug(msg));
}

void StelScriptMgr::scriptEnded()
{
	GETSTELMODULE(StelGui)->setScriptKeys(false);
	mainAPI->getScriptSleeper().setRate(1);
	thread->deleteLater();
	thread=NULL;
	if (engine.hasUncaughtException())
	{
		QString msg = QString("script error: \"%1\" @ line %2").arg(engine.uncaughtException().toString()).arg(engine.uncaughtExceptionLineNumber());
		emit(scriptDebug(msg));
		qWarning() << msg;
	}
	emit(scriptStopped());
}

QMap<QString, QString> StelScriptMgr::mappify(const QStringList& args, bool lowerKey)
{
	QMap<QString, QString> map;
	for(int i=0; i+1<args.size(); i++)
		if (lowerKey)
			map[args.at(i).toLower()] = args.at(i+1);
		else
			map[args.at(i)] = args.at(i+1);
	return map;
}

bool StelScriptMgr::strToBool(const QString& str)
{
	if (str.toLower() == "off" || str.toLower() == "no")
		return false;
	else if (str.toLower() == "on" || str.toLower() == "yes")
		return true;
	return QVariant(str).toBool();
}

bool StelScriptMgr::preprocessScript(QFile& input, QFile& output, const QString& scriptDir)
{
	while (!input.atEnd())
	{
		QString line = QString::fromUtf8(input.readLine());
		QRegExp includeRe("^include\\s*\\(\\s*\"([^\"]+)\"\\s*\\)\\s*;\\s*(//.*)?$");
		if (includeRe.exactMatch(line))
		{
			QString fileName = includeRe.capturedTexts().at(1);
			QString path;

			// Search for the include file.  Rules are:
			// 1. If path is absolute, just use that
			// 2. If path is relative, look in scriptDir + included filename
			if (QFileInfo(fileName).isAbsolute())
				path = fileName;
			else
			{
				try
				{
					path = StelApp::getInstance().getFileMgr().findFile(scriptDir + "/" + fileName);
				}
				catch(std::runtime_error& e)
				{
					qWarning() << "WARNING: script include:" << fileName << e.what();
					return false;
				}
			}

			QFile fic(path);
			bool ok = fic.open(QIODevice::ReadOnly);
			if (ok)
			{
				qDebug() << "script include: " << path;
				preprocessScript(fic, output, scriptDir);
			}
			else
			{
				qWarning() << "WARNING: could not open script include file for reading:" << path;
				return false;
			}
		}
		else
		{
			output.write(line.toUtf8());
		}
	}
	return true;
}

#ifdef ENABLE_STRATOSCRIPT_COMPAT
bool StelScriptMgr::preprocessStratoScript(QFile& input, QFile& output, const QString& scriptDir)
{
	int n=0;
	qDebug() << "Translating stratoscript:";
	while (!input.atEnd())
	{
		QString line = QString::fromUtf8(input.readLine());
		line.replace(QRegExp("#.*$"), "");
		QStringList args = line.split(QRegExp("\\s+"));

		if (args.at(0) == "script")
		{
			if (args.at(1) == "filename")
			{
				QString fileName = args.at(2);
				QString path;

				// Search for the include file.  Rules are:
				// 1. If path is absolute, just use that
				// 2. If path is relative, look in scriptDir + included filename
				if (QFileInfo(fileName).isAbsolute())
					path = fileName;
				else
				{
					try
					{
						path = StelApp::getInstance().getFileMgr().findFile(scriptDir + "/" + fileName);
					}
					catch(std::runtime_error& e)
					{
						qWarning() << "WARNING: script include:" << fileName << e.what();
						return false;
					}
				}

				QFile fic(path);
				bool ok = fic.open(QIODevice::ReadOnly);
				if (ok)
				{
					qDebug() << "script include: " << path;
					preprocessScript(fic, output, scriptDir);
				}
				else
				{
					qWarning() << "WARNING: could not open script include file for reading:" << path;
					return false;
				}
			}
			else
				line = "// untranslated stratoscript (script): " + line;
		}
		else if (args.at(0) == "landscape")
		{
			if (args.at(1) == "load")
				line = QString("LandscapeMgr.setCurrentLandscapeID(\"%1\");").arg(args.at(2));
			else
				line = "// untranslated stratoscript (landscape): " + line;
		}
		else if (args.at(0) == "clear")
		{
			QString state = "natural";
			if (args.at(1) == "state")
				state = args.at(2);

			line = QString("core.clear(\"%1\");").arg(state);
		}
		else if (args.at(0) == "date")
		{
			if (args.at(1).toLower() == "utc")
				line = QString("core.setDate(\"%1\"); ").arg(args.at(2));
			else if (args.at(1) == "local")
				line = QString("core.setDate(\"%1\", \"local\"); ").arg(args.at(2));
			else
				line = "// untranslated stratoscript (date): " + line;
		}
		else if (args.at(0) == "deselect")
			line = "core.selectObjectByName(\"\", false);";
		else if (args.at(0) == "flag")
		{
			if (args.at(1) == "atmosphere")
				line = QString("LandscapeMgr.setFlagAtmosphere(%1);").arg(strToBool(args.at(2)));
			else if (args.at(1) == "azimuthal_grid")
				line = QString("GridLinesMgr.setFlagAzimuthalGrid(%1);").arg(strToBool(args.at(2)));
			else if (args.at(1) == "cardinal_points")
				line = QString("LandscapeMgr.setFlagCardinalsPoints(%1);").arg(strToBool(args.at(2)));
			else if (args.at(1) == "constellation_art")
				line = QString("ConstellationMgr.setFlagArt(%1);").arg(strToBool(args.at(2)));
			else if (args.at(1) == "constellation_boundaries")
				line = QString("ConstellationMgr.setFlagBoundaries(%1);").arg(strToBool(args.at(2)));
			else if (args.at(1) == "constellation_drawing" || args.at(1) == "constellations")
				line = QString("ConstellationMgr.setFlagLines(%1);").arg(strToBool(args.at(2)));
			else if (args.at(1) == "constellation_names")
				line = QString("ConstellationMgr.setFlagLabels(%1);").arg(strToBool(args.at(2)));
			else if (args.at(1) == "constellation_pick")
				line = QString("ConstellationMgr.setFlagIsolateSelected(%1);").arg(strToBool(args.at(2)));
			else if (args.at(1) == "ecliptic_line")
				line = QString("GridLinesMgr.setFlagEclipticLine(%1);").arg(strToBool(args.at(2)));
			else if (args.at(1) == "equator_line")
				line = QString("GridLinesMgr.setFlagEquatorLine(%1);").arg(strToBool(args.at(2)));
			else if (args.at(1) == "equator_grid")
				line = QString("GridLinesMgr.setFlagEquatorGrid(%1);").arg(strToBool(args.at(2)));
			else if (args.at(1) == "fog")
				line = QString("LandscapeMgr.setFlagFog(%1);").arg(strToBool(args.at(2)));
			else if (args.at(1) == "gravity_labels") // TODO when in new script API
				line = "// untranslated stratoscropt (flag gravity_labels): " + line;
			else if (args.at(1) == "moon_scaled")
				line = QString("SolarSystem.setFlagMoonScale(%1);").arg(strToBool(args.at(2)));
			else if (args.at(1) == "landscape")
				line = QString("LandscapeMgr.setFlagLandscape(%1);").arg(strToBool(args.at(2)));
			else if (args.at(1) == "landscape_sets_location")
				line = QString("LandscapeMgr.setFlagLandscapeSetsLocation(%1);").arg(strToBool(args.at(2)));
			else if (args.at(1) == "meridian_line")
				line = QString("GridLinesMgr.setFlagMeridianLine(%1);").arg(strToBool(args.at(2)));
			else if (args.at(1) == "milky_way")
				line = QString("MilkyWay.setFlagShow(%1);").arg(strToBool(args.at(2)));
			else if (args.at(1) == "nebulae")
				line = QString("NebulaMgr.setFlagShow(%1);").arg(strToBool(args.at(2)));
			else if (args.at(1) == "nebula_names")
				line = QString("NebulaMgr.setFlagNames(%1);").arg(strToBool(args.at(2)));
			else if (args.at(1) == "night") // TODO when in new script API
				line = "// untranslated stratoscropt (flag night): " + line;
			else if (args.at(1) == "object_trails")
				line = QString("SolarSystem.setFlagTrails(%1);").arg(strToBool(args.at(2)));
			else if (args.at(1) == "planets")
				line = QString("SolarSystem.setFlagTrails(%1);").arg(strToBool(args.at(2)));
			else if (args.at(1) == "planet_names")
				line = QString("SolarSystem.setFlagLabels(%1);").arg(strToBool(args.at(2)));
			else if (args.at(1) == "planet_orbits")
				line = QString("SolarSystem.setFlagOrbits(%1);").arg(strToBool(args.at(2)));
			else if (args.at(1) == "star_names")
				line = QString("StarMgr.setFlagLabels(%1);").arg(strToBool(args.at(2)));
			else if (args.at(1) == "star_twinkle") // TODO when in new script API
				line = "// untranslated stratoscropt (flag star_twinkle): " + line;
			else if (args.at(1) == "stars")
				line = QString("StarMgr.setFlagStars(%1);").arg(strToBool(args.at(2)));
			else
				line = "// untranslated stratoscript (flag): " + line;
		}
		else if (args.at(0) == "image")
		{
			args.removeFirst();
			QMap<QString, QString> map = mappify(args, true);
			if (map.contains("action"))
			{
				if (!map.contains("name"))
					line = "// untranslated stratoscript (image but no name param): " + line;
				else
				{
					if (map["action"].toLower() == "load")
					{
						if (!map.contains("filename") || !map.contains("scale") || !map.contains("altitude") || !map.contains("azimuth"))
							line = "// untranslated stratoscript (image load but insufficient params): " + line;
						else
						{
							QString coords="viewport";
							int xpos=0;
							int ypos=0;
							bool ok;
							if (map.contains("coordinate_system")) coords = map["coordinate_system"];
							if (map.contains("xpos")) xpos = QVariant(map["xpos"]).toInt(&ok);
							if (!ok || xpos!=1) xpos=0;
							if (map.contains("ypos")) ypos = QVariant(map["ypos"]).toInt(&ok);
							if (!ok || ypos!=1) ypos=0;

							if (coords=="viewport")
							{
								line = QString("ScreenImageMgr.createScreenImage(\"%1\", \"%2\", %3, %4);").arg(map["name"]).arg(map["filename"]).arg(QVariant(map["azimuth"]).toInt()).arg(QVariant(map["altitude"]).toInt());
							}
							else if (coords=="horizontal")
							{
								line = "// untranslated stratoscript (image coords horizontal not implemented yet): " + line; // TODO when we have API for it.
							}
							else
								line = "// untranslated stratoscript (image coords type unknown): " + line;
						}
					}
					else if (map["action"].toLower() == "drop")
						line = QString("ScreenImageMgr.deleteImage(\"%1\")").arg(map["name"]);
					else
						line = "// untranslated stratoscript (image): " + line;
				}
			}
			else
				line = "// untranslated stratoscropt (image): " + line;
		}
		else if (args.at(0) == "select")
		{
			args.removeFirst();
			QMap<QString, QString> map = mappify(args, true);
			bool pointer = false;
			QString objectName;
			if (map.contains("pointer"))
				pointer = QVariant(map["pointer"]).toBool();
			if (map.contains("planet"))
				objectName = map["planet"];
			else if (map.contains("hp"))
				objectName = "HP" + map["hp"];
			else if (map.contains("constellation"))
				objectName = map["constellation"];
			else if (map.contains("constellation_star"))
				objectName = map["constellation_star"];
			else if (map.contains("nebula"))
				objectName = map["nebula"];

			line = QString("core.selectObjectByName(\"%1\", %2);").arg(objectName).arg(pointer);
		}
		else if (args.at(0) == "wait")
		{
			if (args.at(1) == "duration")
				line = QString("core.wait(\"%1\");").arg(args.at(2));
			else
				line = "// untranslated stratoscript (wait): " + line;
		}
		else if (args.at(0) == "zoom")
		{
			bool ok;
			args.removeFirst();
			QMap<QString, QString> map = mappify(args);
			double duration = 1.0;
			if (map.contains("duration"))
				duration = QVariant(map["duration"]).toDouble(&ok);
			if (!ok)
				duration = 1.0;

			if (map.contains("auto"))
			{
				if (map["auto"].toLower() == "in")
					line = QString("StelMovementMgr.autoZoomIn(%1);").arg(duration);
				else if (map["auto"].toLower() == "out")
					line = QString("StelMovementMgr.autoZoomOut(%1);").arg(duration);
				else if (map["auto"].toLower() == "initial")
					line = QString("StelMovementMgr.zoomTo(StelMovementMgr.getInitFov(), %1);").arg(duration);
			}
			else if (map.contains("fov"))
			{
				double fov;
				fov = QVariant(map["fov"]).toDouble(&ok);
				if (ok)
					line = QString("StelMovementMgr.zoomTo(%1, %2);").arg(fov).arg(duration);
				else
					line = "// untranslated stratoscript (zoom fov): " + line;
			}
			else if (map.contains("delta_fov"))
			{
				double dfov;
				dfov = QVariant(map["delta_fov"]).toDouble(&ok);
				if (ok)
					line = QString("StelMovementMgr.zoomTo(%1, %2);").arg(dfov).arg(duration);
				else
					line = "// untranslated stratoscript (zoom delta_fov): " + line;
			}
			else
				line = "// untranslated stratoscript (zoom): " + line;
		}
		else if (args.at(0) == "timerate")
		{
			bool ok;
			if (args.at(1) == "rate")
			{
				double rate = QVariant(args.at(2)).toDouble(&ok);
				if (ok)
					line = QString("core.setTimeRate(%1);").arg(rate);
				else
					line = "// untranslated stratoscript (timerate rate): " + line;
			}
			else
				line = "// untranslated stratoscript (timerate): " + line;
		}
		else
		{
			line = "// untranslated stratoscript: " + line;
		}
		qDebug() << QString("%1: ").arg(++n, 4) + line;
		line += "\n";
		output.write(line.toUtf8());
	}
	return true;
}

#endif // ENABLE_STRATOSCRIPT_COMPAT


