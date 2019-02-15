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


#include "StelScriptOutput.hpp"
#include "StelScriptMgr.hpp"
#include "StelMainScriptAPI.hpp"
#include "StelModuleMgr.hpp"
#include "LabelMgr.hpp"
#include "ScreenImageMgr.hpp"
#include "StelActionMgr.hpp"
#include "StelTranslator.hpp"

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelFileMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelMovementMgr.hpp"

#include "StelSkyDrawer.hpp"
#include "StelSkyLayerMgr.hpp"

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
#include <QtScript>

#include <cmath>

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

class StelScriptEngineAgent : public QScriptEngineAgent
{
public:
	explicit StelScriptEngineAgent(QScriptEngine *engine);
	virtual ~StelScriptEngineAgent() {}

	void setPauseScript(bool pause) { isPaused=pause; }
	bool getPauseScript() { return isPaused; }

	void positionChange(qint64 scriptId, int lineNumber, int columnNumber);

private:
	bool isPaused;
};

StelScriptMgr::StelScriptMgr(QObject *parent): QObject(parent)
{
	engine = new QScriptEngine(this);
	connect(&StelApp::getInstance(), SIGNAL(aboutToQuit()), this, SLOT(stopScript()), Qt::DirectConnection);
	// Scripting images
	ScreenImageMgr* scriptImages = new ScreenImageMgr();
	scriptImages->init();
	StelApp::getInstance().getModuleMgr().registerModule(scriptImages);

	// Allow Vec3f managment in scripts
	qScriptRegisterMetaType(engine, vec3fToScriptValue, vec3fFromScriptValue);
	// Constructor
	QScriptValue ctor = engine->newFunction(createVec3f);
	engine->globalObject().setProperty("Vec3f", ctor);

	// Add the core object to access methods related to core
	mainAPI = new StelMainScriptAPI(this);
	QScriptValue objectValue = engine->newQObject(mainAPI);
	engine->globalObject().setProperty("core", objectValue);

	// Add other classes which we want to be directly accessible from scripts
	if(StelSkyLayerMgr* smgr = GETSTELMODULE(StelSkyLayerMgr))
		objectValue = engine->newQObject(smgr);

	// For accessing star scale, twinkle etc.
	objectValue = engine->newQObject(StelApp::getInstance().getCore()->getSkyDrawer());
	engine->globalObject().setProperty("StelSkyDrawer", objectValue);
	
	setScriptRate(1.0);
	
	engine->setProcessEventsInterval(10);

	agent = new StelScriptEngineAgent(engine);
	engine->setAgent(agent);

	initActions();
}

void StelScriptMgr::initActions()
{
	StelActionMgr* actionMgr = StelApp::getInstance().getStelActionManager();
	QSignalMapper* mapper = new QSignalMapper(this);
	for (const auto script : getScriptList())
	{
		QString shortcut = getShortcut(script);
		QString actionId = "actionScript/" + script;
		StelAction* action = actionMgr->addAction(
					actionId, N_("Scripts"), q_(getName(script).trimmed()), mapper, "map()", shortcut);
		mapper->setMapping(action, script);
	}
	connect(mapper, SIGNAL(mapped(QString)), this, SLOT(runScript(QString)));
}


StelScriptMgr::~StelScriptMgr()
{
}

void StelScriptMgr::addModules() 
{
	// Add all the StelModules into the script engine
	StelModuleMgr* mmgr = &StelApp::getInstance().getModuleMgr();
	for (auto* m : mmgr->getAllModules())
	{
		QScriptValue objectValue = engine->newQObject(m);
		engine->globalObject().setProperty(m->objectName(), objectValue);
	}
}

QStringList StelScriptMgr::getScriptList() const
{
	QStringList scriptFiles;

	QSet<QString> files = StelFileMgr::listContents("scripts", StelFileMgr::File, true);
	QRegExp fileRE("^.*\\.ssc$");
	for (const auto& f : files)
	{
		if (fileRE.exactMatch(f))
			scriptFiles << f;
	}
	return scriptFiles;
}

bool StelScriptMgr::scriptIsRunning() const
{
	return engine->isEvaluating();
}

QString StelScriptMgr::runningScriptId() const
{
	return scriptFileName;
}

QString StelScriptMgr::getHeaderSingleLineCommentText(const QString& s, const QString& id, const QString& notFoundText) const
{
	QFile file(StelFileMgr::findFile("scripts/" + s, StelFileMgr::File));
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QString msg = QString("WARNING: script file %1 could not be opened for reading").arg(QDir::toNativeSeparators(s));
		emit(scriptDebug(msg));
		qWarning() << msg;
		return QString();
	}

	//use a buffered stream instead of QFile::readLine - much faster!
	QTextStream textStream(&file);
	textStream.setCodec("UTF-8");

	QRegExp nameExp("^\\s*//\\s*" + id + ":\\s*(.+)$");
	while (!textStream.atEnd())
	{
		QString line = textStream.readLine();
		if (nameExp.exactMatch(line))
		{
			file.close();
			return nameExp.capturedTexts().at(1).trimmed();
		}
	}
	file.close();
	return notFoundText;
}

QString StelScriptMgr::getHtmlDescription(const QString &s, bool generateDocumentTags) const
{
	QString html;
	if (generateDocumentTags)
		html += "<html><body>";
	html += "<h2>" + q_(getName(s).trimmed()) + "</h2>";
	QString d = getDescription(s).trimmed();
	d.replace("\n", "<br />");
	d.replace(QRegExp("\\s{2,}"), " ");
	html += "<p>" + q_(d) + "</p>";
	html += "<p>";

	QString author = getAuthor(s).trimmed();
	if (!author.isEmpty())
	{
		html += "<strong>" + q_("Author") + "</strong>: " + author + "<br />";
	}
	QString license = getLicense(s).trimmed();
	if (!license.isEmpty())
	{
		html += "<strong>" + q_("License") + "</strong>: " + license + "<br />";
	}
	QString version = getVersion(s).trimmed();
	if (!version.isEmpty())
	{
		html += "<strong>" + q_("Version") + "</strong>: " + version + "<br />";
	}
	QString shortcut = getShortcut(s).trimmed();
	if (!shortcut.isEmpty())
	{
		html += "<strong>" + q_("Shortcut") + "</strong>: " + shortcut;
	}
	html += "</p>";

	if (generateDocumentTags)
		html += "</body></html>";

	return html;
}

QString StelScriptMgr::getName(const QString& s) const
{
	return getHeaderSingleLineCommentText(s, "Name", s);
}

QString StelScriptMgr::getAuthor(const QString& s) const
{
	return getHeaderSingleLineCommentText(s, "Author");
}

QString StelScriptMgr::getLicense(const QString& s) const
{
	return getHeaderSingleLineCommentText(s, "License", "");
}

QString StelScriptMgr::getVersion(const QString& s) const
{
	return getHeaderSingleLineCommentText(s, "Version", "");
}


QString StelScriptMgr::getShortcut(const QString& s) const
{
	return getHeaderSingleLineCommentText(s, "Shortcut", "");
}

QString StelScriptMgr::getDescription(const QString& s) const
{
	QFile file(StelFileMgr::findFile("scripts/" + s, StelFileMgr::File));
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QString msg = QString("WARNING: script file %1 could not be opened for reading").arg(QDir::toNativeSeparators(s));
		emit(scriptDebug(msg));
		qWarning() << msg;
		return QString();
	}

	//use a buffered stream instead of QFile::readLine - much faster!
	QTextStream textStream(&file);
	textStream.setCodec("UTF-8");

	QString desc = "";
	bool inDesc = false;
	QRegExp descExp("^\\s*//\\s*Description:\\s*([^\\s].+)\\s*$");
	QRegExp descNewlineExp("^\\s*//\\s*$");
	QRegExp descContExp("^\\s*//\\s*([^\\s].*)\\s*$");
	while (!textStream.atEnd())
	{
		QString line = textStream.readLine();
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
	return desc.trimmed();
}

bool StelScriptMgr::runPreprocessedScript(const QString &preprocessedScript, const QString& scriptId)
{
	if (engine->isEvaluating())
	{
		QString msg = QString("ERROR: there is already a script running, please wait until it's over.");
		emit(scriptDebug(msg));
		qWarning() << msg;
		return false;
	}

	// Make sure that the gui objects have been completely initialized (there used to be problems with startup scripts).
	Q_ASSERT(StelApp::getInstance().getGui());

	engine->globalObject().setProperty("scriptRateReadOnly", 1.0);

	scriptFileName = scriptId;

	// Notify that the script starts here
	emit(scriptRunning());
	emit runningScriptIdChanged(scriptId);

	// run that script
	engine->evaluate(preprocessedScript);
	scriptEnded();
	return true;
}

// Run the script located at the given location
bool StelScriptMgr::runScript(const QString& fileName, const QString& includePath)
{
	QString preprocessedScript;
	prepareScript(preprocessedScript,fileName,includePath);
	return runPreprocessedScript(preprocessedScript,fileName);
}

bool StelScriptMgr::runScriptDirect(const QString &scriptCode, const QString& includePath)
{
	if(includePath.isNull())
		return runPreprocessedScript(scriptCode, "<Direct script input>");
	else
	{
		QString path = includePath;
		if(includePath.isEmpty())
			path = QStringLiteral("scripts");

		QString processed;
		bool ok = preprocessScript(scriptCode,processed, path);
		if(ok)
			return runPreprocessedScript(processed, "<Direct script input>");
		return false;
	}
}

bool StelScriptMgr::prepareScript(QString &script, const QString &fileName, const QString &includePath)
{
	QString absPath;

	if (QFileInfo(fileName).isAbsolute())
		absPath = fileName;
	else
		absPath = StelFileMgr::findFile("scripts/" + fileName);

	if (absPath.isEmpty())
	{
		QString msg = QString("WARNING: could not find script file %1").arg(QDir::toNativeSeparators(fileName));
		emit(scriptDebug(msg));
		qWarning() << msg;
		return false;
	}

	QString scriptDir = QFileInfo(absPath).dir().path();

	QFile fic(absPath);
	if (!fic.open(QIODevice::ReadOnly))
	{
		QString msg = QString("WARNING: cannot open script: %1").arg(QDir::toNativeSeparators(fileName));
		emit(scriptDebug(msg));
		qWarning() << msg;
		return false;
	}

	if (!includePath.isEmpty())
		scriptDir = includePath;

	bool ok = false;
	if (fileName.endsWith(".ssc"))
		ok = preprocessScript(fic, script, scriptDir);
	if (!ok)
	{
		return false;
	}

	return true;
}

void StelScriptMgr::stopScript()
{
	if (engine->isEvaluating())
	{
		GETSTELMODULE(LabelMgr)->deleteAllLabels();
		GETSTELMODULE(ScreenImageMgr)->deleteAllImages();
		if (agent->getPauseScript()) {
			agent->setPauseScript(false);
		}
		QString msg = QString("INFO: asking running script to exit");
		emit(scriptDebug(msg));
		//qDebug() << msg;
		engine->abortEvaluation();
	}
	scriptEnded();
}

void StelScriptMgr::setScriptRate(float r)
{
	//qDebug() << "StelScriptMgr::setScriptRate(" << r << ")";
	if (!engine->isEvaluating())
	{
		engine->globalObject().setProperty("scriptRateReadOnly", r);
		return;
	}
	
	float currentScriptRate = engine->globalObject().property("scriptRateReadOnly").toNumber();
	
	// pre-calculate the new time rate in an effort to prevent there being much latency
	// between setting the script rate and the time rate.
	float factor = r / currentScriptRate;
	
	StelCore* core = StelApp::getInstance().getCore();
	core->setTimeRate(core->getTimeRate() * factor);
	
	GETSTELMODULE(StelMovementMgr)->setMovementSpeedFactor(core->getTimeRate());
	engine->globalObject().setProperty("scriptRateReadOnly", r);
}

void StelScriptMgr::pauseScript()
{
	agent->setPauseScript(true);
}

void StelScriptMgr::resumeScript()
{
	agent->setPauseScript(false);
}

double StelScriptMgr::getScriptRate() const
{
	return engine->globalObject().property("scriptRateReadOnly").toNumber();
}

void StelScriptMgr::debug(const QString& msg)
{
	emit(scriptDebug(msg));
}

void StelScriptMgr::output(const QString &msg)
{
	StelScriptOutput::writeLog(msg);
	emit(scriptOutput(msg));
}

void StelScriptMgr::resetOutput(void)
{
	StelScriptOutput::reset();
	emit(scriptOutput(""));
}

void StelScriptMgr::saveOutputAs(const QString &filename)
{
	StelScriptOutput::saveOutputAs(filename);
}

void StelScriptMgr::scriptEnded()
{
	if (engine->hasUncaughtException())
	{
		QString msg = QString("script error: \"%1\" @ line %2").arg(engine->uncaughtException().toString()).arg(engine->uncaughtExceptionLineNumber());
		emit(scriptDebug(msg));
		qWarning() << msg;
	}

	GETSTELMODULE(StelMovementMgr)->setMovementSpeedFactor(1.0);
	scriptFileName = QString();
	emit runningScriptIdChanged(scriptFileName);
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
	for (const auto& line : lines)
	{
		if (includeRe.exactMatch(line))
		{
			QString fileName = includeRe.capturedTexts().at(1);
			QString path;

			// Search for the include file.  Rules are:
			// 1. If path is absolute, just use that
			// 2. If path is relative, look in scriptDir + included filename
			// 3. If path is relative (undefined), look in standard scripts directory + included filename
			if (QFileInfo(fileName).isAbsolute())
				path = fileName;
			else
			{
				path = StelFileMgr::findFile(scriptDir + "/" + fileName);
				if (path.isEmpty())
				{
					qWarning() << "WARNING: file not found! Let's check standard scripts directory...";

					// OK, file is not exists in relative path; Let's check standard scripts directory
					path = StelFileMgr::findFile("scripts/" + fileName);

					if (path.isEmpty())
					{
						qWarning() << "WARNING: script include:" << QDir::toNativeSeparators(fileName);
						return false;
					}
				}
			}

			QFile fic(path);
			bool ok = fic.open(QIODevice::ReadOnly);
			if (ok)
			{
				qDebug() << "script include: " << QDir::toNativeSeparators(path);
				preprocessScript(fic, output, scriptDir);
			}
			else
			{
				qWarning() << "WARNING: could not open script include file for reading:" << QDir::toNativeSeparators(path);
				return false;
			}
		}
		else
		{
			output += line;
			output += '\n';
		}
	}

	if (qApp->property("verbose")==true)
	{
		// Debug to find stupid errors. The line usually reported may be off due to the preprocess stage.
		QStringList outputList=output.split('\n');
		qDebug() << "Script after preprocessing:";
		int lineIdx=0;
		for (const auto& line : outputList)
		{
			qDebug() << lineIdx << ":" << line;
			lineIdx++;
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
