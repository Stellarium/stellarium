/*
 * Stellarium
 * Copyright (C) 2009 Matthew Gates
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

#include "StelScriptSyntaxHighlighter.hpp"
#include "VecMath.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelModuleMgr.hpp"
#include "StelScriptMgr.hpp"
#include "StelMainScriptAPI.hpp"

#include <QtWidgets>
#include <QColor>
#include <QSettings>
#include <QMetaMethod>

// Retrieve the set of a module's known public slots and insert into Hash under the given name
void StelScriptSyntaxHighlighter::locateFunctions( const QMetaObject* metaObject, QString scriptName )
{
	QSet<QString> funcs;
	for( int i = metaObject->methodOffset(); i < metaObject->methodCount(); ++i )
	{
		if( metaObject->method(i).methodType() == QMetaMethod::Slot &&
			metaObject->method(i).access() == QMetaMethod::Public )
		{
			QString fn = metaObject->method(i).methodSignature();
			fn.replace(QRegularExpression("\\(.*$"), "");
			funcs << fn;
		}
	}
	mod2funcs.insert( scriptName, funcs );	
}

StelScriptSyntaxHighlighter::StelScriptSyntaxHighlighter(QTextDocument *parent)
	: QSyntaxHighlighter(parent)
{
	setFormats();

	// Highlight object names which can be used in scripting.
	StelModuleMgr* mmgr = &StelApp::getInstance().getModuleMgr();
	for( auto* m : mmgr->getAllModules() )
	{
		locateFunctions( m->metaObject(), m->objectName() );
	}

	// Special case #1: StelMainScriptAPI calls must be done on global "core"
	// StelScriptMgr has private StelMainScriptAPI* mainAPI
	locateFunctions( StelApp::getInstance().getScriptMgr().getMetaOfStelMainScriptAPI(), "core" );

	// Special case #2: StelSkyDrawer
	locateFunctions( StelApp::getInstance().getCore()->getSkyDrawer()->metaObject(), "StelSkyDrawer" );

	// Collect simple rules into highlightingRules. They
	// are well distinguished by their initial character.
	HighlightingRule rule;

	// decimal and hexadecimal numeric constants
	rule.pattern = QRegularExpression("\\b("
			       "\\d+(?:\\.\\d+)?(?:[eE][+-]?(\\d+))?"
			       "|"
			       "0[xX][0-9a-fA-F]+"
			       ")\\b");
	rule.format = &literalFormat;
	highlightingRules.append(rule);
	
	// Three versions of String literals, both ways, and regular expression
	rule.pattern = QRegularExpression("\"[^\"\\\\]*(\\\\.[^\"\\\\]*)*\"");
	rule.format = &literalFormat;
	highlightingRules.append(rule);

	rule.pattern = QRegularExpression("'[^'\\\\]*(\\\\.[^'\\\\]*)*'");
	rule.format = &literalFormat;
	highlightingRules.append(rule);

	rule.pattern = QRegularExpression("/(?!/)[^/\\\\]*(\\\\.[^/\\\\]*)*/[a-z]*"); // Negative lookahead!
	rule.format = &literalFormat;
	highlightingRules.append(rule);


	// Keywords and Predefineds
	for (int i=0; i<keywords.length(); i++)
	{
	    rule.pattern = QRegularExpression(QString("\\b%1\\b").arg(keywords.at(i)));
	    rule.format = &keywordFormat;
	    highlightingRules.append(rule);
	}
	for (int i=0; i<predefineds.length(); i++)
	{
	    rule.pattern = QRegularExpression(QString("\\b%1\\b").arg(predefineds.at(i)));
	    rule.format = &predefFormat;
	    highlightingRules.append(rule);
	}
	// Our own functions: highlight bad method names, mark valid method names, then mark modules
	QHashIterator<QString, QSet<QString>> hIt(mod2funcs);
	while (hIt.hasNext())
	{
	    hIt.next();

	    // bad method names
	    rule.pattern = QRegularExpression(QString("\\b%1\\s*\\.\\s*\\w*\\b").arg(hIt.key()));
	    rule.format = &noMethFormat;
	    highlightingRules.append(rule);

	    //qDebug() << hIt.key() << "has following values:";
	    // valid method names
	    if (hIt.value().count()>0)
	    {
		QSetIterator<QString>sIt(hIt.value());
		while (sIt.hasNext()) {
		    //qDebug() << "\t" << sIt.peekNext();
		    rule.pattern = QRegularExpression(QString("\\b%1\\s*\\.\\s*%2\\b") .arg(hIt.key(), sIt.next()));
		    rule.format = &methodFormat;
		    highlightingRules.append(rule);
		}
	    }
	    // modules
	    rule.pattern = QRegularExpression(QString("\\b%1\\b").arg(hIt.key()));
	    rule.format = &moduleFormat;
	    highlightingRules.append(rule);
	}

	// Finally, line comments. If anything has been highlighted so far, this wins over all...
	rule.pattern = QRegularExpression("//[^\n]*");
	rule.format = &commentFormat;
	highlightingRules.append(rule);

	//qDebug() << "We have " << highlightingRules.length() << "Rules";
}

void StelScriptSyntaxHighlighter::setFormats(void)
{
	QColor col;
	QString defaultColor = "0.8,0.8,0.8";
	QSettings* conf = StelApp::getInstance().getSettings();
	const QString section = StelApp::getInstance().getVisionModeNight() ? "night_color" : "color";

	// comments
	col = Vec3f(conf->value(section + "/script_console_comment_color", defaultColor).toString()).toQColor();
	commentFormat.setForeground(col);

	// ECMAscript reserved words
	col = Vec3f(conf->value(section + "/script_console_keyword_color", defaultColor).toString()).toQColor();
	keywordFormat.setForeground(col);
	keywordFormat.setFontWeight(QFont::Bold);

	// predefined names
	QString predefDefault = "0.7,0.0,0.7";
	col = Vec3f(conf->value(section + "/script_console_predefined_color", predefDefault).toString()).toQColor();
	predefFormat.setForeground(col);
	predefFormat.setFontWeight(QFont::Bold);
	
	// Stellarium object names which can be used in scripting and their methods
	col = Vec3f(conf->value(section + "/script_console_module_color", defaultColor).toString()).toQColor();
	moduleFormat.setForeground(col);
	moduleFormat.setFontWeight(QFont::Bold);
	methodFormat.setForeground(col);
	methodFormat.setFontWeight(QFont::Bold);
	methodFormat.setFontItalic( true );

	// literals
	col = Vec3f(conf->value(section + "/script_console_constant_color", defaultColor).toString()).toQColor();
	literalFormat.setForeground(col);
	literalFormat.setFontWeight(QFont::Bold);

	// function calls
	col = Vec3f(conf->value(section + "/script_console_function_color", defaultColor).toString()).toQColor();
	functionFormat.setForeground(col);
	functionFormat.setFontItalic(true);

	// call of an unknown method
	QString unknownDefault = "1.0,0.0,0.0";
	col = Vec3f(conf->value(section + "/script_console_unknown_color", unknownDefault).toString()).toQColor();
	noMethFormat.setForeground( col );
	noMethFormat.setFontItalic( true );
}

void StelScriptSyntaxHighlighter::highlightBlock(const QString &text)
{
	// Function call: detect valid identifier with opening bracket.
	QRegularExpression functionPat("\\b[A-Za-z_][A-Za-z0-9_]*\\s*\\(");
	QRegularExpressionMatchIterator it = functionPat.globalMatch(text);
	while (it.hasNext())
	{
		QRegularExpressionMatch match = it.next();
		setFormat(match.capturedStart(), match.capturedLength()-1, functionFormat);
	}

	// process rules which had been defined in the constructor.
	for (const HighlightingRule &rule: qAsConst(highlightingRules))
	{
		it= rule.pattern.globalMatch(text);
		while (it.hasNext())
		{
			QRegularExpressionMatch match=it.next();
			setFormat(match.capturedStart(), match.capturedLength(), *rule.format);
		}
	}

	// Finally, apply the Qt Example for a multiline comment block /*...*/
	QRegularExpression startExpression("/\\*");
	QRegularExpression endExpression("\\*/");

	setCurrentBlockState(0);
	int startIndex = 0;
	if (previousBlockState() != 1)
		startIndex = text.indexOf(startExpression);

	while (startIndex >= 0)
	{
		QRegularExpressionMatch endMatch;
		int endIndex = text.indexOf(endExpression, startIndex, &endMatch);
		int commentLength;
		if (endIndex == -1)
		{
			setCurrentBlockState(1);
			commentLength = text.length() - startIndex;
		}
		else
		{
			commentLength = endIndex - startIndex
				    + endMatch.capturedLength();
	}
	setFormat(startIndex, commentLength, commentFormat);
	startIndex = text.indexOf(startExpression, startIndex + commentLength);
    }
}

// ECMAscript reserved words              // 2015
// display using keywordFormat
const QStringList StelScriptSyntaxHighlighter::keywords = {
    "break", "case", "catch",             // class, const
    "continue",                           // debugger
    "default", "delete", "do", "else",    // export, extends
    "finally", "for", "function", "if",	  // import
    "in", "instanceof", "new", "return",  // super
    "switch", "this", "throw", "try",
    "typeof", "var", "void", "while", "with" };

// ECMAscript predefined stuff (more in 2015)
// display using predefFormat
const QStringList StelScriptSyntaxHighlighter::predefineds = {
    "false", "null", "true",              // constants
    "undefined", "Infinity", "NaN",       // properties
    "arguments", "get", "set",            // identifiers
    "Array", "Boolean", "Date",           // types
    "Function", "Math", "Number",
    "Object", "RegExp", "String",
    "Error", "EvalError", "RangeError",   // errors
    "ReferenceError", "SyntaxError",
    "TypeError", "URIError" };
