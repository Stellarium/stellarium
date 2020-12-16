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
#include "MarkerMgr.hpp"
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
#include "StelUtils.hpp"

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

// 3f - 3f - 3f - 3f - 3f - 3f - 3f - 3f - 3f - 3f - 3f - 3f - 3f - 3f

/***
The C++ type Vec3f is an array of three floats, used for representing
colors according to the terms of the RGB color model. It is mapped to
a JavaScript class with a constructor Vec3f, properties 'r', 'g' and 'b',
and methods toString and toHex. The latter returns a string according to
the pattern '#hhhhhh' (six hexadecimal digits). A second constructor,
Color, is provided as a convenience. Both constructors may be called without
argument to return  'white', a single argument which is either a color name
or a '#hhhhhh' string, or three arguments, specifying the RGB values,
floating point numbers between 0 and 1. Color names correspond to the names
defined in https://www.w3.org/TR/SVG11/types.htm.
***/

QScriptValue vec3fToString(QScriptContext* context, QScriptEngine *engine)
{
	Q_UNUSED(engine)
	QScriptValue that = context->thisObject();
	QScriptValue rVal = that.property( "r", QScriptValue::ResolveLocal );
	QScriptValue gVal = that.property( "g", QScriptValue::ResolveLocal );
	QScriptValue bVal = that.property( "b", QScriptValue::ResolveLocal );
	return "[r:" + rVal.toString() + ", " + "g:" + gVal.toString() + ", " +
            "b:" + bVal.toString() + "]";
}

QScriptValue vec3fToHex(QScriptContext* context, QScriptEngine *engine)
{
	Q_UNUSED(engine)
	QScriptValue that = context->thisObject();
	QScriptValue rVal = that.property( "r", QScriptValue::ResolveLocal );
	QScriptValue gVal = that.property( "g", QScriptValue::ResolveLocal );
	QScriptValue bVal = that.property( "b", QScriptValue::ResolveLocal );
	return QString("#%1%2%3")
		.arg(qMin(255, int(rVal.toNumber() * 255)), 2, 16, QChar('0'))
		.arg(qMin(255, int(gVal.toNumber() * 255)), 2, 16, QChar('0'))
		.arg(qMin(255, int(bVal.toNumber() * 255)), 2, 16, QChar('0'));
}

void vec3fFromScriptValue(const QScriptValue& obj, Vec3f& c)
{
	c[0] = static_cast<float>(obj.property("r").toNumber());
	c[1] = static_cast<float>(obj.property("g").toNumber());
	c[2] = static_cast<float>(obj.property("b").toNumber());
}

QScriptValue vec3fToScriptValue(QScriptEngine *engine, const Vec3f& c)
{
	QScriptValue obj = engine->newObject();
	obj.setProperty("r", QScriptValue(engine, static_cast<qsreal>(c[0])));
	obj.setProperty("g", QScriptValue(engine, static_cast<qsreal>(c[1])));
	obj.setProperty("b", QScriptValue(engine, static_cast<qsreal>(c[2])));
	obj.setProperty("toString", engine->newFunction( vec3fToString ));
	obj.setProperty("toHex",    engine->newFunction( vec3fToHex ));
	return obj;
}

// Constructor Vec3f
// Three arguments are r, g, b. One argument is "#hhhhhh", or, perhaps, a color name.
QScriptValue createVec3f(QScriptContext* context, QScriptEngine *engine)
{
	Vec3f c;
	switch( context->argumentCount() )
	{
		case 0:
			// no color: black or white? Let's say: white, against a black sky.
			c.set( 1, 1, 1 );
			break;
		case 1:
			// either '#hhhhhh' or a color name - let QColor do the work
			if( context->argument(0).isString() )
			{
				QColor qcol = QColor( context->argument(0).toString() );
				if( qcol.isValid() )
				{
					c.set( static_cast<float>(qcol.redF()), static_cast<float>(qcol.greenF()), static_cast<float>(qcol.blueF()) );
					break;
				}
				else
					context->throwError( QString("Color: invalid color name") );
			}
			else
			{
				// Let's hope: a Color/Vec3f object
				return context->argument(0);
			}
			break;
		case 3:
			// r, g, b: between 0 and 1.
			if( context->argument(0).isNumber() &&
			    context->argument(1).isNumber() &&
			    context->argument(2).isNumber() )
			{
				c[0] = static_cast<float>(context->argument(0).toNumber());
				c[1] = static_cast<float>(context->argument(1).toNumber());
				c[2] = static_cast<float>(context->argument(2).toNumber());
				if( c[0] < 0 || 1 < c[0] ||
				    c[1] < 0 || 1 < c[1] ||
				    c[2] < 0 || 1 < c[2] )
				{
					context->throwError( QString("Color: RGB value out of range [0,1]") );
				}
			}
			break;
		default:
			context->throwError( QString("Color: invalid number of arguments") );
	}
	return vec3fToScriptValue(engine, c);
}

QScriptValue createColor(QScriptContext* context, QScriptEngine *engine)
{
	return engine->globalObject().property("Vec3f").construct(context->argumentsObject());
}

// 3d - 3d - 3d - 3d - 3d - 3d - 3d - 3d - 3d - 3d - 3d - 3d - 3d - 3d

/***
The C++ type Vec3d is an array of three doubles, used for representing
vectors (points or directions) in three-dimensional space. The coordinate
system is implied by the context.  It is mapped to a JavaScript class with
the constructor Vec3d, properties 'x', 'y' and 'z' (and, for historical
reasons, also named 'r', 'g' and 'b') and a method toString. The 
constructor may be called without an argument, returning the null vector,
with three arguments to set the three properties, and with two arguments,
interpreted to mean longtitude and latitude angles (or azimuth and
altitude angles). These angles may be given as double values for degrees,
or as string values denoting angles according to the patterns accepted
by StelUtils::getDecAngle. 
***/


QScriptValue vec3dToString(QScriptContext* context, QScriptEngine *engine)
{
	Q_UNUSED(engine)
	QScriptValue that = context->thisObject();
	QScriptValue xVal = that.property( "r", QScriptValue::ResolveLocal );
	QScriptValue yVal = that.property( "g", QScriptValue::ResolveLocal );
	QScriptValue zVal = that.property( "b", QScriptValue::ResolveLocal );
	return "[" + xVal.toString() + ", " + yVal.toString() + ", " +
		         zVal.toString() + "]";
}

QScriptValue getX(QScriptContext* context, QScriptEngine *engine)
{
	Q_UNUSED(engine)
	QScriptValue that = context->thisObject();
	return that.property( "r", QScriptValue::ResolveLocal );
}
QScriptValue getY(QScriptContext* context, QScriptEngine *engine)
{
	Q_UNUSED(engine)
	QScriptValue that = context->thisObject();
	return that.property( "g", QScriptValue::ResolveLocal );
}
QScriptValue getZ(QScriptContext* context, QScriptEngine *engine)
{
	Q_UNUSED(engine)
	QScriptValue that = context->thisObject();
	return that.property( "b", QScriptValue::ResolveLocal );
}

QScriptValue setX(QScriptContext* context, QScriptEngine *engine)
{
	Q_UNUSED(engine)
	QScriptValue that = context->thisObject();
	that.setProperty("r", context->argument(0).toNumber());
	return QScriptValue();
}
QScriptValue setY(QScriptContext* context, QScriptEngine *engine)
{
	Q_UNUSED(engine)
	QScriptValue that = context->thisObject();
	that.setProperty("g", context->argument(0).toNumber());
	return QScriptValue();
}
QScriptValue setZ(QScriptContext* context, QScriptEngine *engine)
{
	Q_UNUSED(engine)
	QScriptValue that = context->thisObject();
	that.setProperty("b", context->argument(0).toNumber());
	return QScriptValue();
}

void vec3dFromScriptValue(const QScriptValue& obj, Vec3d& c)
{
	c[0] = obj.property("r").toNumber();
	c[1] = obj.property("g").toNumber();
	c[2] = obj.property("b").toNumber();
}

QScriptValue vec3dToScriptValue(QScriptEngine *engine, const Vec3d& v)
{
	QScriptValue obj = engine->newObject();
	obj.setProperty("r", QScriptValue(engine, static_cast<qsreal>(v[0])));
	obj.setProperty("g", QScriptValue(engine, static_cast<qsreal>(v[1])));
	obj.setProperty("b", QScriptValue(engine, static_cast<qsreal>(v[2])));
	obj.setProperty("toString", engine->newFunction( vec3dToString ) );
	obj.setProperty("x", engine->newFunction(getX), QScriptValue::PropertyGetter);
	obj.setProperty("x", engine->newFunction(setX), QScriptValue::PropertySetter);
	obj.setProperty("y", engine->newFunction(getY), QScriptValue::PropertyGetter);
	obj.setProperty("y", engine->newFunction(setY), QScriptValue::PropertySetter);
	obj.setProperty("z", engine->newFunction(getZ), QScriptValue::PropertyGetter);
	obj.setProperty("z", engine->newFunction(setZ), QScriptValue::PropertySetter);	
	return obj;
}

QScriptValue createVec3d(QScriptContext* context, QScriptEngine *engine)
{
	Vec3d c;
	switch( context->argumentCount() )
	{
		case 0:
			c.set( 0, 0, 0 );
			break;
		case 2:
			// longitude/azimuth, latitude/altitude - maybe string dms, maybe numeric degrees
			double lng;
			if( context->argument(0).isString() )
				lng = StelUtils::getDecAngle( context->argument(0).toString() );
			else
				lng = static_cast<double>(context->argument(0).toNumber())*M_PI_180;

			double lat;
			if ( context->argument(1).isString())
				lat = StelUtils::getDecAngle( context->argument(1).toString() );
			else
				lat = static_cast<double>(context->argument(1).toNumber())*M_PI_180;

			StelUtils::spheToRect( lng, lat, c );
			break;
		case 3:
			c[0] = static_cast<double>(context->argument(0).toNumber());
			c[1] = static_cast<double>(context->argument(1).toNumber());
			c[2] = static_cast<double>(context->argument(2).toNumber());
			break;
		default:
			context->throwError( QString("Vec3d: invalid number of arguments") );
	}
	return vec3dToScriptValue(engine, c);
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

void StelScriptMgr::defVecClasses(QScriptEngine *engine)
{
	// Allow Vec3f management in scripts
	qScriptRegisterMetaType(engine, vec3fToScriptValue, vec3fFromScriptValue);
	QScriptValue ctorVec3f = engine->newFunction(createVec3f);
	engine->globalObject().setProperty("Vec3f", ctorVec3f);
	engine->globalObject().setProperty("Color", engine->newFunction(createColor));

	// Allow Vec3d management in scripts
	qScriptRegisterMetaType(engine, vec3dToScriptValue, vec3dFromScriptValue);
	QScriptValue ctorVec3d = engine->newFunction(createVec3d);
	engine->globalObject().setProperty("Vec3d", ctorVec3d);
}

StelScriptMgr::StelScriptMgr(QObject *parent): QObject(parent)
{
	waitEventLoop = new QEventLoop();
	engine = new QScriptEngine(this);
	connect(&StelApp::getInstance(), SIGNAL(aboutToQuit()), this, SLOT(stopScript()), Qt::DirectConnection);
	// Scripting images
	ScreenImageMgr* scriptImages = new ScreenImageMgr();
	scriptImages->init();
	StelApp::getInstance().getModuleMgr().registerModule(scriptImages);

	defVecClasses(engine);

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
	
	engine->setProcessEventsInterval(1); // was 10, let's allow a smoother script execution

	agent = new StelScriptEngineAgent(engine);
	engine->setAgent(agent);

	initActions();
}

void StelScriptMgr::initActions()
{
	StelActionMgr* actionMgr = StelApp::getInstance().getStelActionManager();
	for (const auto& script : getScriptList())
	{
		QString shortcut = getShortcut(script);
		QString actionId = "actionScript/" + script;
		actionMgr->addAction(actionId, N_("Scripts"), q_(getName(script).trimmed()),
				     this, [this, script] { runScript(script); }, shortcut);
	}
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
			return nameExp.cap(1).trimmed();
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
			desc = descExp.cap(1) + " ";
			desc.replace("\n","");
		}
		else if (inDesc)
		{
			QString d("");
			if (descNewlineExp.exactMatch(line))
				d = "\n";
			else if (descContExp.exactMatch(line))
			{
				d = descContExp.cap(1) + " ";
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

	// run that script in a new context
	QScriptContext *context = engine->pushContext();
	engine->evaluate(preprocessedScript);
	engine->popContext();
	scriptEnded();
	Q_UNUSED(context)
	return true;
}

// Run the script located at the given location
bool StelScriptMgr::runScript(const QString& fileName, const QString& includePath)
{
	QString preprocessedScript;
	prepareScript(preprocessedScript,fileName,includePath);
	return runPreprocessedScript(preprocessedScript,fileName);
}

bool StelScriptMgr::runScriptDirect(const QString scriptId, const QString &scriptCode, int &errLoc, const QString& includePath)
{
	QString path = includePath.isNull() ? QString(".") : includePath;
	if( path.isEmpty() )
		path = QStringLiteral("scripts");
	QString processed;
	bool ok = preprocessScript( scriptId, scriptCode, processed, path, errLoc );
	if(ok)
		return runPreprocessedScript(processed, "<Direct script input>");
	return false;
}

bool StelScriptMgr::prepareScript( QString &script, const QString &fileName, const QString &includePath)
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
		ok = preprocessFile(absPath, fic, script, scriptDir);
	if (!ok)
	{
		return false;
	}

	return true;
}

void StelScriptMgr::stopScript()
{
	// Hack abusing stopScript for two different stops: it
	// will be called again after exit from the timer loop
	// which we kill here if it is running.
	if( waitEventLoop->isRunning() )
	{
		waitEventLoop->exit( 1 );
		return;
	}
	
	if (engine->isEvaluating())
	{
		GETSTELMODULE(LabelMgr)->deleteAllLabels();
		GETSTELMODULE(MarkerMgr)->deleteAllMarkers();
		GETSTELMODULE(ScreenImageMgr)->deleteAllImages();
		if (agent->getPauseScript()) {
			agent->setPauseScript(false);
		}
		QString msg = QString("INFO: asking running script to exit");
		emit(scriptDebug(msg));
		//qDebug() << msg;
		engine->abortEvaluation();
	}
	// "Script finished..." is emitted after return from engine->evaluate().
}

void StelScriptMgr::setScriptRate(double r)
{
	//qDebug() << "StelScriptMgr::setScriptRate(" << r << ")";
	if (!engine->isEvaluating())
	{
		engine->globalObject().setProperty("scriptRateReadOnly", r);
		return;
	}
	
	qsreal currentScriptRate = engine->globalObject().property("scriptRateReadOnly").toNumber();
	
	// pre-calculate the new time rate in an effort to prevent there being much latency
	// between setting the script rate and the time rate.
	qsreal factor = r / currentScriptRate;
	
	StelCore* core = StelApp::getInstance().getCore();
	core->setTimeRate(core->getTimeRate() * factor);
	
	GETSTELMODULE(StelMovementMgr)->setMovementSpeedFactor(static_cast<float>(core->getTimeRate()));
	engine->globalObject().setProperty("scriptRateReadOnly", r);
}

void StelScriptMgr::pauseScript()
{
	emit(scriptPaused());
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
		int outputPos = engine->uncaughtExceptionLineNumber();
		QString realPos = lookup( outputPos );
		QString msg = QString("script error: \"%1\" @ line %2").arg(engine->uncaughtException().toString()).arg(realPos);
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


bool StelScriptMgr::preprocessFile(const QString fileName, QFile &input, QString& output, const QString& scriptDir)
{
	QString aText = QString::fromUtf8(input.readAll());
	int errLoc;
	return preprocessScript( fileName, aText, output, scriptDir, errLoc );
}

bool StelScriptMgr::preprocessScript(const QString fileName, const QString &input, QString &output, const QString &scriptDir, int &errLoc )
{
	// The one and only top-level call for expand.
	scriptFileName = fileName;
	num2loc = QMap<int,QPair<QString,int>>();
	outline = 0;
	includeSet.clear();
	errLoc = -1;
	expand( fileName, input, output, scriptDir, errLoc );
	if( errLoc != -1 ){
		return false;
	} else {
		return true;
	}
}
	
void StelScriptMgr::expand(const QString fileName, const QString &input, QString &output, const QString &scriptDir, int &errLoc){
	QStringList lines = input.split("\n");
	QRegExp includeRe("^include\\s*\\(\\s*\"([^\"]+)\"\\s*\\)\\s*;\\s*(//.*)?$");
	int curline = 0;
	for (const auto& line : lines)
	{
		curline++;
		if (includeRe.exactMatch(line))
		{
			QString incName = includeRe.cap(1);
			QString incPath;

			// Search for the include file.  Rules are:
			// 1. If incPath is absolute, just use that
			// 2. If incPath is relative, look in scriptDir + included filename
			// 3. If incPath is relative (undefined), look in standard scripts directory + included filename
			if (QFileInfo(incName).isAbsolute())
				incPath = incName;
			else
			{
				incPath = StelFileMgr::findFile(scriptDir + "/" + incName);
				if (incPath.isEmpty())
				{
					QString fail = scriptDir + "/" + incName;
					qWarning() << "WARNING: file not found! Let's check standard scripts directory...";

					// OK, file is not exists in relative path; Let's check standard scripts directory
					incPath = StelFileMgr::findFile("scripts/" + incName);

					if (incPath.isEmpty())
					{
						fail += " or scripts/" + incName;
						emit(scriptDebug(QString("WARNING: could not find script include file: %1").arg(QDir::toNativeSeparators(incName))));
						qWarning() << "WARNING: could not find script include file: " << QDir::toNativeSeparators(incName);
						if( errLoc == -1 ) errLoc = output.length();
						output += line + " // <<< " + fail + " not found\n";
						outline++;
						continue;
					}
				}
			}
			// Write the include line as a comment. It is end of a range.
			output += "// " + line + "\n";
			outline++;

			if( ! includeSet.contains(incPath) )
			{
				includeSet.insert(incPath);
				num2loc.insert( outline, QPair<QString,int>(fileName, curline) );
				QFile fic(incPath);
				bool ok = fic.open(QIODevice::ReadOnly);
				if (ok)
				{
					qWarning() << "script include: " << QDir::toNativeSeparators(incPath);
					QString aText = QString::fromUtf8(fic.readAll());
					expand( incPath, aText, output, scriptDir, errLoc );
					fic.close();
				}
				else
				{
					emit(scriptDebug(QString("WARNING: could not open script include file for reading: %1").arg(QDir::toNativeSeparators(incPath))));
					qWarning() << "WARNING: could not open script include file for reading: " << QDir::toNativeSeparators(incPath);
				   	if( errLoc == -1 ) errLoc = output.length();
					output += line + " // <<< " + incPath + ": cannot open\n";
					outline++;
					continue;
				}
			}
		}
		else
		{
			output += line + '\n';
			outline++;
		}
	}
	num2loc.insert( outline, QPair<QString,int>(fileName, curline) );

	// Do we need this any more? (WL, 2020-04-10)
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
	return;
}

QString StelScriptMgr::lookup( int outputPos )
{
	// Error beyond the last line?
	if (outputPos > outline)
		outputPos = outline;

	QMap<int,QPair<QString,int>>::const_iterator i = num2loc.lowerBound( outputPos );
	int outputEnd = i.key();
	QString path = i.value().first;
	int inputEnd = i.value().second;
	int inputPos = outputPos - outputEnd + inputEnd;
	// qDebug() << outputPos << " maps to " << path << ":" << inputPos;
	QString msg;
	if (path.isEmpty())
		msg = QString( "%1" ).arg(inputPos);
	else
		msg = QString( "%1:%2" ).arg(path).arg(inputPos);

	return msg;
}

StelScriptEngineAgent::StelScriptEngineAgent(QScriptEngine *engine) 
	: QScriptEngineAgent(engine)
	, isPaused(false)
	{}

void StelScriptEngineAgent::positionChange(qint64 scriptId, int lineNumber, int columnNumber)
{
	Q_UNUSED(scriptId)
	Q_UNUSED(lineNumber)
	Q_UNUSED(columnNumber)

	while (isPaused) {
		// TODO : sleep for 'processEventsInterval' time
		QCoreApplication::processEvents();
	}
}

