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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */


#include "StelScriptMgr.hpp"
#include "StelMainScriptAPI.hpp"
#include "StelModuleMgr.hpp"
#include "LabelMgr.hpp"
#include "ScreenImageMgr.hpp"

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelFileMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelMovementMgr.hpp"

#include "StelSkyDrawer.hpp"
#include "StelSkyLayerMgr.hpp"

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

StelScriptMgr::StelScriptMgr(QObject *parent): QObject(parent)
{
	// Scripting images
	ScreenImageMgr* scriptImages = new ScreenImageMgr();
	scriptImages->init();
	StelApp::getInstance().getModuleMgr().registerModule(scriptImages);

	// Allow Vec3f managment in scripts
	qScriptRegisterMetaType(&engine, vec3fToScriptValue, vec3fFromScriptValue);
	// Constructor
	QScriptValue ctor = engine.newFunction(createVec3f);
	engine.globalObject().setProperty("Vec3f", ctor);

	// Add the core object to access methods related to core
	mainAPI = new StelMainScriptAPI(this);
	QScriptValue objectValue = engine.newQObject(mainAPI);
	engine.globalObject().setProperty("core", objectValue);

	engine.evaluate("function mywait__(sleepDurationSec) {"
					"if (sleepDurationSec<0) return;"
					"var date = new Date();"
					"var curDate = null;"
					"do {curDate = new Date();}"
					"    while(curDate-date < sleepDurationSec*1000/scriptRateReadOnly);}");
	engine.evaluate("core['wait'] = mywait__;");
	
	//! Waits until a specified simulation date/time.  This function
	//! will take into account the rate (and direction) in which simulation
	//! time is passing. e.g. if a future date is specified and the
	//! time is moving backwards, the function will return immediately.
	//! If the time rate is 0, the function will not wait.  This is to
	//! prevent infinite wait time.
	//! @param dt the date string to use
	//! @param spec "local" or "utc"
	engine.evaluate("function mywaitFor__(dt, spec) {if (!spec) spec=\"utc\";"
	"	var JD = core.jdFromDateString(dt, spec);"
	"	var timeSpeed = core.getTimeRate();"
	"	if (timeSpeed == 0.) {core.debug(\"waitFor called with no time passing - would be infinite. not waiting!\"); return;}"
	"	if (timeSpeed > 0) {core.wait((JD-core.getJDay())*timeSpeed);}"
	"	else {core.wait((core.getJDay()-JD)*timeSpeed);}}");
	engine.evaluate("core['waitFor'] = mywaitFor__;");
	
	// Add other classes which we want to be directly accessible from scripts
	if(StelSkyLayerMgr* smgr = GETSTELMODULE(StelSkyLayerMgr))
		objectValue = engine.newQObject(smgr);

	// For accessing star scale, twinkle etc.
	objectValue = engine.newQObject(StelApp::getInstance().getCore()->getSkyDrawer());
	engine.globalObject().setProperty("StelSkyDrawer", objectValue);
	
	setScriptRate(1.0);
	
	engine.setProcessEventsInterval(10);

	agent = new StelScriptEngineAgent(&engine);
	engine.setAgent(agent);
}


StelScriptMgr::~StelScriptMgr()
{
}

void StelScriptMgr::addModules() 
{
	// Add all the StelModules into the script engine
	StelModuleMgr* mmgr = &StelApp::getInstance().getModuleMgr();
	foreach (StelModule* m, mmgr->getAllModules())
	{
		QScriptValue objectValue = engine.newQObject(m);
		engine.globalObject().setProperty(m->objectName(), objectValue);
	}

}

QStringList StelScriptMgr::getScriptList()
{
	QStringList scriptFiles;
	try
	{
		QSet<QString> files = StelFileMgr::listContents("scripts", StelFileMgr::File, true);
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

bool StelScriptMgr::scriptIsRunning()
{
	return engine.isEvaluating();
}

QString StelScriptMgr::runningScriptId()
{
	if (engine.isEvaluating())
		return scriptFileName;
	else
		return QString();
}

const QString StelScriptMgr::getHeaderSingleLineCommentText(const QString& s, const QString& id, const QString& notFoundText)
{
	try
	{
		QFile file(StelFileMgr::findFile("scripts/" + s, StelFileMgr::File));
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			QString msg = QString("WARNING: script file %1 could not be opened for reading").arg(s);
			emit(scriptDebug(msg));
			qWarning() << msg;
			return QString();
		}

		QRegExp nameExp("^\\s*//\\s*" + id + ":\\s*(.+)$");
		while (!file.atEnd())
		{
			QString line = QString::fromUtf8(file.readLine());
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
		QFile file(StelFileMgr::findFile("scripts/" + s, StelFileMgr::File));
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
		while (!file.atEnd())
		{
			QString line = QString::fromUtf8(file.readLine());
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

bool StelScriptMgr::runPreprocessedScript(const QString &preprocessedScript)
{
	if (engine.isEvaluating())
	{
		QString msg = QString("ERROR: there is already a script running, please wait that it's over.");
		emit(scriptDebug(msg));
		qWarning() << msg;
		return false;
	}
	// Seed the PRNG so that script random numbers aren't always the same sequence
	qsrand(QDateTime::currentDateTime().toTime_t());

	// Make sure that the gui object have been completely initialized (there used to be problems with startup scripts).
	Q_ASSERT(StelApp::getInstance().getGui());

	engine.globalObject().setProperty("scriptRateReadOnly", 1.0);

	// Notify that the script starts here
	emit(scriptRunning());

	// run that script
	engine.evaluate(preprocessedScript);
	scriptEnded();
	return true;
}

// Run the script located at the given location
bool StelScriptMgr::runScript(const QString& fileName, const QString& includePath)
{
	QString absPath;
	QString scriptDir;
	try
	{
		if (QFileInfo(fileName).isAbsolute())
			absPath = fileName;
		else
			absPath = StelFileMgr::findFile("scripts/" + fileName);

		scriptDir = QFileInfo(absPath).dir().path();
	}
	catch (std::runtime_error& e)
	{
		QString msg = QString("WARNING: could not find script file %1: %2").arg(fileName).arg(e.what());
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
		return false;
	}

	scriptFileName = fileName;
	if (!includePath.isEmpty())
		scriptDir = includePath;
	QString preprocessedScript;
	bool ok = false;
	if (fileName.endsWith(".ssc"))
		ok = preprocessScript(fic, preprocessedScript, scriptDir);
#ifdef ENABLE_STRATOSCRIPT_COMPAT
	else if (fileName.endsWith(".sts"))
		ok = preprocessStratoScript(fic, preprocessedScript, scriptDir);
#endif
	if (!ok)
	{
		return false;
	}
	return runPreprocessedScript(preprocessedScript);
}

void StelScriptMgr::stopScript()
{
	if (engine.isEvaluating())
	{
		GETSTELMODULE(LabelMgr)->deleteAllLabels();
		GETSTELMODULE(ScreenImageMgr)->deleteAllImages();
		if (agent->getPauseScript()) {
			agent->setPauseScript(false);
		}
		QString msg = QString("INFO: asking running script to exit");
		emit(scriptDebug(msg));
		//qDebug() << msg;
		engine.abortEvaluation();
	}
	scriptEnded();
}

void StelScriptMgr::setScriptRate(float r)
{
	//qDebug() << "StelScriptMgr::setScriptRate(" << r << ")";
	if (!engine.isEvaluating())
	{
		engine.globalObject().setProperty("scriptRateReadOnly", r);
		return;
	}
	
	float currentScriptRate = engine.globalObject().property("scriptRateReadOnly").toNumber();
	
	// pre-calculate the new time rate in an effort to prevent there being much latency
	// between setting the script rate and the time rate.
	float factor = r / currentScriptRate;
	
	StelCore* core = StelApp::getInstance().getCore();
	core->setTimeRate(core->getTimeRate() * factor);
	
	GETSTELMODULE(StelMovementMgr)->setMovementSpeedFactor(core->getTimeRate());
	engine.globalObject().setProperty("scriptRateReadOnly", r);

}

void StelScriptMgr::pauseScript() {
	agent->setPauseScript(true);
}

void StelScriptMgr::resumeScript() {
	agent->setPauseScript(false);
}

double StelScriptMgr::getScriptRate()
{
	return engine.globalObject().property("scriptRateReadOnly").toNumber();
}

void StelScriptMgr::debug(const QString& msg)
{
	emit(scriptDebug(msg));
}

void StelScriptMgr::scriptEnded()
{
	if (engine.hasUncaughtException())
	{
		// NOTE: engine.uncaughtExceptionLineNumber() gives incorrect line number (probably Qt bug). Correct line number can be
		// calculate as (engine.uncaughtExceptionLineNumber()+1)/2
		// More info: https://bugs.launchpad.net/stellarium/+bug/1046518 and
		// http://sourceforge.net/projects/stellarium/forums/forum/278769/topic/5591465
		QString msg = QString("script error: \"%1\" @ line %2").arg(engine.uncaughtException().toString()).arg((engine.uncaughtExceptionLineNumber() + 1)/2);
		emit(scriptDebug(msg));
		qWarning() << msg;
	}

	GETSTELMODULE(StelMovementMgr)->setMovementSpeedFactor(1.0);
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

bool StelScriptMgr::preprocessScript(const QString &input, QString &output, const QString &scriptDir)
{
	QStringList lines = input.split("\n", QString::SkipEmptyParts);
	QRegExp includeRe("^include\\s*\\(\\s*\"([^\"]+)\"\\s*\\)\\s*;\\s*(//.*)?$");
	foreach (QString line, lines)
	{
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
					path = StelFileMgr::findFile(scriptDir + "/" + fileName);
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
			output += line;
			output += '\n';
		}
	}
	return true;
}


bool StelScriptMgr::preprocessScript(QFile &input, QString& output, const QString& scriptDir)
{
	QString s = QString::fromUtf8(input.readAll());
	return preprocessScript(s, output, scriptDir);
}

StelScriptEngineAgent::StelScriptEngineAgent(QScriptEngine *engine) 
	: QScriptEngineAgent(engine)
{
	isPaused = false;
}

void StelScriptEngineAgent::positionChange(qint64 scriptId, int lineNumber, int columnNumber)
{
	Q_UNUSED(scriptId);
	Q_UNUSED(lineNumber);
	Q_UNUSED(columnNumber);

	while (isPaused) {
		// TODO : sleep for 'processEventsInterval' time
		QCoreApplication::processEvents();
	}
}
