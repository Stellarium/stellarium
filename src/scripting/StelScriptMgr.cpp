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


#include "StelScriptMgr.hpp"
#include "StelMainScriptAPI.hpp"

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelFileMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelMovementMgr.hpp"
#include "StelNavigator.hpp"
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
			Q_ASSERT(StelApp::getInstance().getGui());
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
	if(StelSkyLayerMgr* smgr = GETSTELMODULE(StelSkyLayerMgr))
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

