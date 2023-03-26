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

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>
#include <QStringList>
#include <QTemporaryFile>
#include <QVariant>

#include <cmath>


#ifdef ENABLE_SCRIPT_QML
#include <QJSEngine>

void StelScriptMgr::defVecClasses(QJSEngine *engine)
{
	qRegisterMetaType<V3d>();
	QMetaType::registerConverter<V3d,QString>(&V3d::toString);
	QMetaType::registerConverter<V3d, Vec3d>(&V3d::toVec3d);
	//QMetaType::registerConverter<Vec3d, V3d>(&V3d::fromVec3d);
	QJSValue v3dMetaObject = engine->newQMetaObject(&V3d::staticMetaObject);
	engine->globalObject().setProperty("V3d", v3dMetaObject);

	qRegisterMetaType<V3f>();
	QMetaType::registerConverter<V3f,QString>(&V3f::toString);
	QMetaType::registerConverter<V3f, Vec3f>(&V3f::toVec3f);
	//QMetaType::registerConverter<Vec3f, V3f>(&V3f::fromVec3f);
	QJSValue v3fMetaObject = engine->newQMetaObject(&V3f::staticMetaObject);
	engine->globalObject().setProperty("V3f", v3fMetaObject);

	qRegisterMetaType<Color>();
	QMetaType::registerConverter<Color,QString>(&Color::toRGBString);
	QMetaType::registerConverter<Color, Vec3f>(&Color::toVec3f);
	//QMetaType::registerConverter<Vec3f, Color>(&Color::fromVec3f);
	QJSValue colorMetaObject = engine->newQMetaObject(&Color::staticMetaObject);
	engine->globalObject().setProperty("Color", colorMetaObject);
}

#else
#include <QtScript>
// 3f - 3f - 3f - 3f - 3f - 3f - 3f - 3f - 3f - 3f - 3f - 3f - 3f - 3f

/***
The C++ type Vec3f is an array of three floats, used for representing
colors according to the terms of the RGB color model. In Qt5-based versions it is mapped to
a JavaScript class with a constructor Vec3f, properties 'r', 'g' and 'b',
and methods toString and toHex. The latter returns a string according to
the pattern '#hhhhhh' (six hexadecimal digits). A second constructor,
Color, is provided as a convenience. Both constructors may be called without
argument to return  'white', a single argument which is either a color name
or a '#hhhhhh' string, or three arguments, specifying the RGB values,
floating point numbers between 0 and 1. Color names correspond to the names
defined in https://www.w3.org/TR/SVG11/types.htm.

This functionality is however deprecated. We strongly advise not to use it in scripts.
Scripts using it will only work on Qt5-based builds of Stellarium (series 0.*)
***/

static QScriptValue vec3fToString(QScriptContext* context, QScriptEngine *engine)
{
	Q_UNUSED(engine)
	QScriptValue that = context->thisObject();
	QScriptValue rVal = that.property( "r", QScriptValue::ResolveLocal );
	QScriptValue gVal = that.property( "g", QScriptValue::ResolveLocal );
	QScriptValue bVal = that.property( "b", QScriptValue::ResolveLocal );
	return "[r:" + rVal.toString() + ", " + "g:" + gVal.toString() + ", " +
            "b:" + bVal.toString() + "]";
}

static QScriptValue vec3fToHex(QScriptContext* context, QScriptEngine *engine)
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

// This just ensures that Color.toVec3f does nothing in Qt5/QtScript
static QScriptValue vec3fNop(QScriptContext* context, QScriptEngine *engine)
{
	Q_UNUSED(engine)
	return context->thisObject();
}

static QScriptValue vec3fGetR(QScriptContext* context, QScriptEngine *engine)
{
	Q_UNUSED(engine)
	QScriptValue that = context->thisObject();
	QScriptValue rVal = that.property( "r", QScriptValue::ResolveLocal );
	return rVal;
}

static QScriptValue vec3fGetG(QScriptContext* context, QScriptEngine *engine)
{
	Q_UNUSED(engine)
	QScriptValue that = context->thisObject();
	QScriptValue rVal = that.property( "g", QScriptValue::ResolveLocal );
	return rVal;
}

static QScriptValue vec3fGetB(QScriptContext* context, QScriptEngine *engine)
{
	Q_UNUSED(engine)
	QScriptValue that = context->thisObject();
	QScriptValue rVal = that.property( "b", QScriptValue::ResolveLocal );
	return rVal;
}

// This does not work.
static QScriptValue vec3fSetR(QScriptContext* context, QScriptEngine *engine)
{
	Q_UNUSED(engine)
	qDebug() << "setR() does not work. Create a new Color.";
	QScriptValue callee = context->callee();
	if (context->argumentCount() == 1) // writing?
		callee.setProperty("r", context->argument(0));
	return callee.property("r");
}
// This does not work.
static QScriptValue vec3fSetG(QScriptContext* context, QScriptEngine *engine)
{
	qDebug() << "setG() does not work. Create a new Color.";
	//qDebug() << "setG called. argcount=" << context->argumentCount() << "value=" << context->argument(0).toNumber();
	//Q_UNUSED(engine)
	QScriptValue callee = context->callee();
	if (context->argumentCount() == 1) // writing?
		callee.setProperty("g", QScriptValue(engine, static_cast<qsreal>(context->argument(0).toNumber())));
	return callee.property("g");
}
// This does not work.
static QScriptValue vec3fSetB(QScriptContext* context, QScriptEngine *engine)
{
	Q_UNUSED(engine)
	qDebug() << "setB() does not work. Create a new Color.";
	QScriptValue callee = context->callee();
	if (context->argumentCount() == 1) // writing?
		callee.setProperty("b", context->argument(0));
	return callee.property("b");
}

static void vec3fFromScriptValue(const QScriptValue& obj, Vec3f& c)
{
	c[0] = static_cast<float>(obj.property("r").toNumber());
	c[1] = static_cast<float>(obj.property("g").toNumber());
	c[2] = static_cast<float>(obj.property("b").toNumber());
}

static QScriptValue vec3fToScriptValue(QScriptEngine *engine, const Vec3f& c)
{
	QScriptValue obj = engine->newObject();
	obj.setProperty("r", QScriptValue(engine, static_cast<qsreal>(c[0])));
	obj.setProperty("g", QScriptValue(engine, static_cast<qsreal>(c[1])));
	obj.setProperty("b", QScriptValue(engine, static_cast<qsreal>(c[2])));
	obj.setProperty("toString", engine->newFunction( vec3fToString ));
	obj.setProperty("toRGBString", engine->newFunction( vec3fToString ));  // alias, compatibility to Qt6 scripting
	obj.setProperty("toHex",    engine->newFunction( vec3fToHex ));
	obj.setProperty("toVec3f",  engine->newFunction( vec3fNop )); // do nothing. Useful for a Color object. Compatibility for Qt6 scripting.
	obj.setProperty("getR",     engine->newFunction( vec3fGetR ));
	obj.setProperty("getG",     engine->newFunction( vec3fGetG ));
	obj.setProperty("getB",     engine->newFunction( vec3fGetB ));
	obj.setProperty("setR",     engine->newFunction( vec3fSetR, 1 )); // does nothing but emits a warning.
	obj.setProperty("setG",     engine->newFunction( vec3fSetG, 1 )); // does nothing but emits a warning.
	obj.setProperty("setB",     engine->newFunction( vec3fSetB, 1 )); // does nothing but emits a warning.
	return obj;
}

// Constructor Vec3f
// Three arguments are r, g, b. One argument is "#hhhhhh", or, perhaps, a color name.
static QScriptValue createVec3f(QScriptContext* context, QScriptEngine *engine)
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

static QScriptValue createColor(QScriptContext* context, QScriptEngine *engine)
{
	return engine->globalObject().property("Vec3f").construct(context->argumentsObject());
}

// 3d - 3d - 3d - 3d - 3d - 3d - 3d - 3d - 3d - 3d - 3d - 3d - 3d - 3d

/***
The C++ type Vec3d is an array of three doubles, used for representing
vectors (points or directions) in three-dimensional space. The coordinate
system is implied by the context.  In Qt5-based versions it is mapped to a JavaScript class with
the constructor Vec3d, properties 'x', 'y' and 'z' (and, for historical
reasons, also named 'r', 'g' and 'b') and a method toString. The 
constructor may be called without an argument, returning the null vector,
with three arguments to set the three properties, and with two arguments,
interpreted to mean longtitude and latitude angles (or azimuth and
altitude angles). These angles may be given as double values for degrees,
or as string values denoting angles according to the patterns accepted
by StelUtils::getDecAngle. 


This functionality is however deprecated. We strongly advise not to use it in scripts.
Scripts using it will only work on Qt5-based builds of Stellarium (series 0.*)
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
	qWarning() << "The Vec3d script object is deprecated and will not work in future versions of Stellarium. Use V3d.";
	Q_UNUSED(engine)
	QScriptValue that = context->thisObject();
	return that.property( "r", QScriptValue::ResolveLocal );
}
QScriptValue getY(QScriptContext* context, QScriptEngine *engine)
{
	qWarning() << "The Vec3d script object is deprecated and will not work in future versions of Stellarium. Use V3d.";
	Q_UNUSED(engine)
	QScriptValue that = context->thisObject();
	return that.property( "g", QScriptValue::ResolveLocal );
}
QScriptValue getZ(QScriptContext* context, QScriptEngine *engine)
{
	qWarning() << "The Vec3d script object is deprecated and will not work in future versions of Stellarium. Use V3d.";
	Q_UNUSED(engine)
	QScriptValue that = context->thisObject();
	return that.property( "b", QScriptValue::ResolveLocal );
}

QScriptValue setX(QScriptContext* context, QScriptEngine *engine)
{
	qWarning() << "The Vec3d script object is deprecated and will not work in future versions of Stellarium. Use V3d.";
	Q_UNUSED(engine)
	QScriptValue that = context->thisObject();
	that.setProperty("r", context->argument(0).toNumber());
	return QScriptValue();
}
QScriptValue setY(QScriptContext* context, QScriptEngine *engine)
{
	qWarning() << "The Vec3d script object is deprecated and will not work in future versions of Stellarium. Use V3d.";
	Q_UNUSED(engine)
	QScriptValue that = context->thisObject();
	that.setProperty("g", context->argument(0).toNumber());
	return QScriptValue();
}
QScriptValue setZ(QScriptContext* context, QScriptEngine *engine)
{
	qWarning() << "The Vec3d script object is deprecated and will not work in future versions of Stellarium. Use V3d.";
	Q_UNUSED(engine)
	QScriptValue that = context->thisObject();
	that.setProperty("b", context->argument(0).toNumber());
	return QScriptValue();
}

void vec3dFromScriptValue(const QScriptValue& obj, Vec3d& c)
{
	qWarning() << "The Vec3d script object is deprecated and will not work in future versions of Stellarium. Use V3d.";
	c[0] = obj.property("r").toNumber();
	c[1] = obj.property("g").toNumber();
	c[2] = obj.property("b").toNumber();
}

QScriptValue vec3dToScriptValue(QScriptEngine *engine, const Vec3d& v)
{
	qWarning() << "The Vec3d script object is deprecated and will not work in future versions of Stellarium. Use V3d.";
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
	qWarning() << "The Vec3d script object is deprecated and will not work in future versions of Stellarium. Use core.vec3d() or a V3d object.";
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
	virtual ~StelScriptEngineAgent() Q_DECL_OVERRIDE {}

	void setPauseScript(bool pause) { qWarning() << "setPauseScript() is deprecated and will no longer be available in future versions of Stellarium."; isPaused=pause; }
	bool getPauseScript() { qWarning() << "getPauseScript() is deprecated and will no longer be available in future versions of Stellarium."; return isPaused; }

	void positionChange(qint64 scriptId, int lineNumber, int columnNumber) Q_DECL_OVERRIDE;

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

	// Configure the new replacement types so that new scripts should work also with the old engine.

	qRegisterMetaType<V3d>();
	QMetaType::registerConverter<V3d,QString>(&V3d::toString);
	QMetaType::registerConverter<V3d, Vec3d>(&V3d::toVec3d);
	//QMetaType::registerConverter<Vec3d, V3d>(&V3d::fromVec3d);
	QScriptValue v3dMetaObject = engine->newQMetaObject(&V3d::staticMetaObject);
	engine->globalObject().setProperty("V3d", v3dMetaObject);

	qRegisterMetaType<V3f>();
	QMetaType::registerConverter<V3f,QString>(&V3f::toString);
	QMetaType::registerConverter<V3f, Vec3f>(&V3f::toVec3f);
	//QMetaType::registerConverter<Vec3f, V3f>(&V3f::fromVec3f);
	QScriptValue v3fMetaObject = engine->newQMetaObject(&V3f::staticMetaObject);
	engine->globalObject().setProperty("V3f", v3fMetaObject);
}
#endif

StelScriptMgr::StelScriptMgr(QObject *parent): QObject(parent)
{
	waitEventLoop = new QEventLoop();
#ifdef ENABLE_SCRIPT_QML
	engine = new QJSEngine(this);
	engine->installExtensions(QJSEngine::ConsoleExtension); // TBD: Maybe remove as unnecessary for us?
#else
	engine = new QScriptEngine(this);
	// This is enough for a simple Array access for a QVector<int> input or return type (e.g. Calendars plugin)
	qScriptRegisterSequenceMetaType<QVector<int>>(engine); // no longer needed with QJSEngine!
#endif
	connect(&StelApp::getInstance(), SIGNAL(aboutToQuit()), this, SLOT(stopScript()), Qt::DirectConnection);

	// Scripting images
	ScreenImageMgr* scriptImages = new ScreenImageMgr();
	scriptImages->init();
	StelApp::getInstance().getModuleMgr().registerModule(scriptImages);

	defVecClasses(engine); // Install handling of Vec3d[deprecated]/Vec3f[deprecated]/V3d/V3f/Color classes

	// Add the core object to access methods related to core
	mainAPI = new StelMainScriptAPI(this);
#ifdef ENABLE_SCRIPT_QML
	QJSValue objectValue = engine->newQObject(mainAPI);
	mainAPI->setEngine(engine);
#else
	QScriptValue objectValue = engine->newQObject(mainAPI);
#endif
	engine->globalObject().setProperty("core", objectValue);

	// Add other classes which we want to be directly accessible from scripts
	if(StelSkyLayerMgr* smgr = GETSTELMODULE(StelSkyLayerMgr))
		objectValue = engine->newQObject(smgr);

	// For accessing star scale, twinkle etc.
	objectValue = engine->newQObject(StelApp::getInstance().getCore()->getSkyDrawer());
	engine->globalObject().setProperty("StelSkyDrawer", objectValue);

	setScriptRate(1.0);

#ifndef ENABLE_SCRIPT_QML
	engine->setProcessEventsInterval(1); // was 10, let's allow a smoother script execution

	agent = new StelScriptEngineAgent(engine);
	engine->setAgent(agent);
#endif
	initActions();
	QSettings *conf=StelApp::getInstance().getSettings();
	flagAllowExternalScreenshotDir=conf->value("scripts/flag_allow_screenshots_dir", false).toBool();
	flagAllowWriteAbsolutePaths=conf->value("scripts/flag_allow_write_absolute_path", false).toBool();
}

void StelScriptMgr::initActions()
{
	StelActionMgr* actionMgr = StelApp::getInstance().getStelActionManager();
	const QStringList scriptList = getScriptList();
	for (const auto& script : scriptList)
	{
		QString shortcut = getShortcut(script);
		QString actionId = "actionScript/" + script;
		actionMgr->addAction(actionId, N_("Scripts"), q_(getName(script).trimmed()),
				     this, [this, script] { runScript(script); }, shortcut);
	}
}

StelScriptMgr::~StelScriptMgr()
{
#ifdef ENABLE_SCRIPT_QML
	engine->collectGarbage();
	delete engine;
#else
	delete engine; // We never did that before?
#endif
}

void StelScriptMgr::addModules() 
{
	// Add all the StelModules into the script engine
	StelModuleMgr* mmgr = &StelApp::getInstance().getModuleMgr();
	const QList allModules = mmgr->getAllModules();
	for (auto* m : allModules)
	{
		#ifdef ENABLE_SCRIPT_QML
			QJSValue objectValue = engine->newQObject(m);
		#else
			QScriptValue objectValue = engine->newQObject(m);
		#endif
		engine->globalObject().setProperty(m->objectName(), objectValue);
	}
}

void StelScriptMgr::addObject(QObject *obj)
{
	#ifdef ENABLE_SCRIPT_QML
		QJSValue objectValue = engine->newQObject(obj);
	#else
		QScriptValue objectValue = engine->newQObject(obj);
	#endif
	engine->globalObject().setProperty(obj->objectName(), objectValue);
}

QStringList StelScriptMgr::getScriptList() const
{
	QStringList scriptFiles;

	QSet<QString> files = StelFileMgr::listContents("scripts", StelFileMgr::File, true);
	static const QRegularExpression fileRE("^.*\\.ssc$");
	for (const auto& f : files)
	{
		if (fileRE.match(f).hasMatch())
			scriptFiles << f;
	}
	return scriptFiles;
}

#ifdef ENABLE_SCRIPT_QML
bool StelScriptMgr::scriptIsRunning()
{
	if (mutex.tryLock())
	{
		mutex.unlock();
		return false;
	}
	else
		return true;
#else
bool StelScriptMgr::scriptIsRunning() const
{
	return engine->isEvaluating();
#endif
}

QString StelScriptMgr::runningScriptId() const
{
	return scriptFileName;
}

QString StelScriptMgr::getHeaderSingleLineCommentText(const QString& s, const QString& id, const QString& notFoundText)
{
	QFile file(StelFileMgr::findFile("scripts/" + s, StelFileMgr::File));
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QString msg = QString("WARNING: script file %1 could not be opened for reading").arg(QDir::toNativeSeparators(s));
		emit scriptDebug(msg);
		qWarning() << msg;
		return QString();
	}

	//use a buffered stream instead of QFile::readLine - much faster!
	QTextStream textStream(&file);
#if (QT_VERSION>=QT_VERSION_CHECK(6,0,0))
	textStream.setEncoding(QStringConverter::Utf8);
#else
	textStream.setCodec("UTF-8");
#endif
	QRegularExpression nameExp("^\\s*//\\s*" + id + ":\\s*(.+)$");
	while (!textStream.atEnd())
	{
		QString line = textStream.readLine();
		QRegularExpressionMatch nameMatch=nameExp.match(line);
		if (nameMatch.hasMatch())
		{
			file.close();
			return nameMatch.captured(1).trimmed();
		}
	}
	file.close();
	return notFoundText;
}

QString StelScriptMgr::getHtmlDescription(const QString &s, bool generateDocumentTags)
{
	QString html;
	if (generateDocumentTags)
		html += "<html><body>";
	html += "<h2>" + q_(getName(s).trimmed()) + "</h2>";
	QString d = getDescription(s).trimmed();
	d.replace("\n", "<br />");
	static const QRegularExpression spaceExp("\\s{2,}");
	d.replace(spaceExp, " ");
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

QString StelScriptMgr::getName(const QString& s)
{
	return getHeaderSingleLineCommentText(s, "Name", s);
}

QString StelScriptMgr::getAuthor(const QString& s)
{
	return getHeaderSingleLineCommentText(s, "Author");
}

QString StelScriptMgr::getLicense(const QString& s)
{
	return getHeaderSingleLineCommentText(s, "License", "");
}

QString StelScriptMgr::getVersion(const QString& s)
{
	return getHeaderSingleLineCommentText(s, "Version", "");
}


QString StelScriptMgr::getShortcut(const QString& s)
{
	return getHeaderSingleLineCommentText(s, "Shortcut", "");
}

QString StelScriptMgr::getDescription(const QString& s)
{
	QFile file(StelFileMgr::findFile("scripts/" + s, StelFileMgr::File));
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QString msg = QString("WARNING: script file %1 could not be opened for reading").arg(QDir::toNativeSeparators(s));
		emit scriptDebug(msg);
		qWarning() << msg;
		return QString();
	}

	//use a buffered stream instead of QFile::readLine - much faster!
	QTextStream textStream(&file);
#if (QT_VERSION>=QT_VERSION_CHECK(6,0,0))
	textStream.setEncoding(QStringConverter::Utf8);
#else
	textStream.setCodec("UTF-8");
#endif
	QString desc = "";
	bool inDesc = false;
	static const QRegularExpression descExp("^\\s*//\\s*Description:\\s*([^\\s].+)\\s*$");
	static const QRegularExpression descNewlineExp("^\\s*//\\s*$");
	static const QRegularExpression descContExp("^\\s*//\\s*([^\\s].*)\\s*$");
	while (!textStream.atEnd())
	{
		QString line = textStream.readLine();
		QRegularExpressionMatch descMatch=descExp.match(line);
		if (!inDesc && descMatch.hasMatch())
		{
			inDesc = true;
			desc = descMatch.captured(1) + " ";
			desc.replace("\n","");
		}
		else if (inDesc)
		{
			QString d("");
			QRegularExpressionMatch descContMatch=descContExp.match(line);
			if (descNewlineExp.match(line).hasMatch())
				d = "\n";
			else if (descContMatch.hasMatch())
			{
				d = descContMatch.captured(1) + " ";
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
#ifdef ENABLE_SCRIPT_QML
	if (!mutex.tryLock())
#else
	if (engine->isEvaluating())
#endif
	{
		QString msg = QString("ERROR: there is already a script running, please wait until it's over.");
		emit scriptDebug(msg);
		qWarning() << msg;
		return false;
	}

	// Make sure that the gui objects have been completely initialized (there used to be problems with startup scripts).
	Q_ASSERT(StelApp::getInstance().getGui());

	engine->globalObject().setProperty("scriptRateReadOnly", 1.0);

	scriptFileName = scriptId;

	// Notify that the script starts here
	emit scriptRunning();
	emit runningScriptIdChanged(scriptId);

#ifdef ENABLE_SCRIPT_QML
	engine->setInterrupted(false);
	//QStringList stackTrace;
	//result=engine->evaluate(preprocessedScript, QString(), 1, &stackTrace);
	result=engine->evaluate(preprocessedScript, QString(), 1);
	scriptEnded();
#else
	// run that script in a new context
	QScriptContext *context = engine->pushContext();
	engine->evaluate(preprocessedScript);
	engine->popContext();
	scriptEnded();
	Q_UNUSED(context)
#endif
	return true;
}

// Run the script located at the given location
bool StelScriptMgr::runScript(const QString& fileName, const QString& includePath)
{
	QString preprocessedScript;
	if (prepareScript(preprocessedScript,fileName,includePath))
		return runPreprocessedScript(preprocessedScript,fileName);
	else
		return false;
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

bool StelScriptMgr::runScriptDirect(const QString& scriptCode, const QString &includePath)
{
	QString dummyId("directScript");
	int dummyErrLoc;
	return runScriptDirect(dummyId, scriptCode, dummyErrLoc, includePath);
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
		emit scriptDebug(msg);
		qWarning() << msg;
		return false;
	}

	QString scriptDir = QFileInfo(absPath).dir().path();

	QFile fic(absPath);
	if (!fic.open(QIODevice::ReadOnly))
	{
		QString msg = QString("WARNING: cannot open script: %1").arg(QDir::toNativeSeparators(fileName));
		emit scriptDebug(msg);
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
	
#ifdef ENABLE_SCRIPT_QML
	if (!mutex.tryLock())
	{
		engine->setInterrupted(true);
		GETSTELMODULE(LabelMgr)->deleteAllLabels();
		GETSTELMODULE(MarkerMgr)->deleteAllMarkers();
		GETSTELMODULE(ScreenImageMgr)->deleteAllImages();
		QString msg = QString("INFO: asking running script to exit");
		emit scriptDebug(msg);
		//qDebug() << msg;
	}
	else
		mutex.unlock(); // all was OK, no script was running.
#else
	if (engine->isEvaluating())
	{
		GETSTELMODULE(LabelMgr)->deleteAllLabels();
		GETSTELMODULE(MarkerMgr)->deleteAllMarkers();
		GETSTELMODULE(ScreenImageMgr)->deleteAllImages();
		if (agent->getPauseScript()) {
			agent->setPauseScript(false);
		}
		QString msg = QString("INFO: asking running script to exit");
		emit scriptDebug(msg);
		//qDebug() << msg;
		engine->abortEvaluation();
	}
	// "Script finished..." is emitted after return from engine->evaluate().
#endif
}

void StelScriptMgr::setScriptRate(double r)
{
	//qDebug() << "StelScriptMgr::setScriptRate(" << r << ")";
#ifdef ENABLE_SCRIPT_QML
	if (mutex.tryLock())
	{
		engine->globalObject().setProperty("scriptRateReadOnly", r);
		mutex.unlock();
		return;
	}
#else
	if (!engine->isEvaluating())
	{
		engine->globalObject().setProperty("scriptRateReadOnly", r);
		return;
	}
#endif
	qreal currentScriptRate = engine->globalObject().property("scriptRateReadOnly").toNumber();
	
	// pre-calculate the new time rate in an effort to prevent there being much latency
	// between setting the script rate and the time rate.
	qreal factor = r / currentScriptRate;
	
	StelCore* core = StelApp::getInstance().getCore();
	core->setTimeRate(core->getTimeRate() * factor);
	
	GETSTELMODULE(StelMovementMgr)->setMovementSpeedFactor(static_cast<float>(core->getTimeRate()));
	engine->globalObject().setProperty("scriptRateReadOnly", r);
}

void StelScriptMgr::pauseScript()
{
#ifdef ENABLE_SCRIPT_QML
	qWarning() << "pauseScript() is no longer available and does nothing.";
#else
	qWarning() << "pauseScript() is deprecated and will no longer be available in future versions of Stellarium.";
	emit scriptPaused();
	agent->setPauseScript(true);
#endif
}

void StelScriptMgr::resumeScript()
{
#ifdef ENABLE_SCRIPT_QML
	qWarning() << "resumeScript() is no longer available and does nothing.";
#else
	qWarning() << "resumeScript() is deprecated and will no longer be available in future versions of Stellarium.";
	agent->setPauseScript(false);
#endif
}

double StelScriptMgr::getScriptRate() const
{
	return engine->globalObject().property("scriptRateReadOnly").toNumber();
}

void StelScriptMgr::debug(const QString& msg)
{
	emit scriptDebug(msg);
}

void StelScriptMgr::output(const QString &msg)
{
	StelScriptOutput::writeLog(msg);
	emit scriptOutput(msg);
}

void StelScriptMgr::resetOutput(void)
{
	StelScriptOutput::reset();
	emit scriptOutput("");
}

void StelScriptMgr::saveOutputAs(const QString &filename)
{
	StelScriptOutput::saveOutputAs(filename);
}

void StelScriptMgr::scriptEnded()
{
#ifdef ENABLE_SCRIPT_QML
	if (result.isError())
	{
		static const QMap<QJSValue::ErrorType, QString>errorMap={
			{QJSValue::GenericError, "Generic"},
			{QJSValue::RangeError, "Range"},
			{QJSValue::ReferenceError, "Reference"},
			{QJSValue::SyntaxError, "Syntax"},
			{QJSValue::TypeError, "Type"},
			{QJSValue::URIError, "URI"}};
		QString msg = QString("script error: '%1'  @ line %2: %3").arg(errorMap.value(result.errorType()), result.property("lineNumber").toString(), result.toString());
		emit scriptDebug(msg);
		qWarning() << msg;
		qWarning() << "Error name:" << result.property("name").toString() << "message" << result.property("message").toString()
			   << "fileName" << result.property("fileName").toString() << "lineNumber" << result.property("lineNumber").toString() << "stack" << result.property("stack").toString();
	}
	mutex.unlock();
#else
	if (engine->hasUncaughtException())
	{
		int outputPos = engine->uncaughtExceptionLineNumber();
		QString realPos = lookup( outputPos );
		QString msg = QString("script error: \"%1\" @ line %2").arg(engine->uncaughtException().toString(), realPos);
		emit scriptDebug(msg);
		qWarning() << msg;
	}
#endif
	GETSTELMODULE(StelMovementMgr)->setMovementSpeedFactor(1.0);
	scriptFileName = QString();
	emit runningScriptIdChanged(scriptFileName);
	emit scriptStopped();
}

//QMap<QString, QString> StelScriptMgr::mappify(const QStringList& args, bool lowerKey)
//{
//	QMap<QString, QString> map;
//	for(int i=0; i+1<args.size(); i++)
//		if (lowerKey)
//			map[args.at(i).toLower()] = args.at(i+1);
//		else
//			map[args.at(i)] = args.at(i+1);
//	return map;
//}

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
	static const QRegularExpression includeRe("^include\\s*\\(\\s*\"([^\"]+)\"\\s*\\)\\s*;\\s*(//.*)?$");
	int curline = 0;
	for (const auto& line : lines)
	{
		curline++;
		QRegularExpressionMatch includeMatch=includeRe.match(line);
		if (includeMatch.hasMatch())
		{
			QString incName = includeMatch.captured(1);
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
						emit scriptDebug(QString("WARNING: could not find script include file: %1").arg(QDir::toNativeSeparators(incName)));
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
					emit scriptDebug(QString("WARNING: could not open script include file for reading: %1").arg(QDir::toNativeSeparators(incPath)));
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

#ifndef ENABLE_SCRIPT_QML
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
#endif

void StelScriptMgr::setFlagAllowExternalScreenshotDir(bool flag)
{
	if (flag!=flagAllowExternalScreenshotDir)
	{
		flagAllowExternalScreenshotDir=flag;
		QSettings* conf = StelApp::getInstance().getSettings();
		conf->setValue("scripts/flag_allow_screenshots_dir", flag);
		conf->sync();
		emit flagAllowExternalScreenshotDirChanged(flag);
	}
}
void StelScriptMgr::setFlagAllowWriteAbsolutePaths(bool flag)
{
	if (flag!=flagAllowWriteAbsolutePaths)
	{
		flagAllowWriteAbsolutePaths=flag;
		QSettings* conf = StelApp::getInstance().getSettings();
		conf->setValue("scripts/flag_allow_write_absolute_path", flag);
		conf->sync();
		emit flagAllowWriteAbsolutePathsChanged(flag);
	}
}
