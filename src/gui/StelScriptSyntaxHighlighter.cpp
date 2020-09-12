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
			fn.replace(QRegExp("\\(.*$"), ""); 
			funcs << fn;
		}
	}
	mod2funcs.insert( scriptName, funcs );	
}

StelScriptSyntaxHighlighter::StelScriptSyntaxHighlighter(QTextDocument *parent)
	: QSyntaxHighlighter(parent)
{
	setFormats();

	// ECMAscript reserved words              // 2015
	// display using keywordFormat
	keywords = {
		"break", "case", "catch",             // class, const
		"continue",                           // debugger
		"default", "delete", "do", "else",    // export, extends
		"finally", "for", "function", "if",	  // import
		"in", "instanceof", "new", "return",  // super
		"switch", "this", "throw", "try",
		"typeof", "var", "void", "while", "with" };

	// ECMAscript predefined stuff (more in 2015)
 	// display using predefFormat
	predefineds = {
		"false", "null", "true",              // constants
		"undefined", "Infinity", "NaN",       // properties
		"arguments", "get", "set",            // identifiers
		"Array", "Boolean", "Date",           // types
		"Function", "Math", "Number",
		"Object", "RegExp", "String", 
		"Error", "EvalError", "RangeError",   // errors
		"ReferenceError", "SyntaxError",
		"TypeError", "URIError" };

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

	// Identifier pattern
	identPat = QRegExp("^\\b[A-Za-z_][A-Za-z_0-9]*\\b");

	// Function call
	functionPat = QRegExp("^\\s*\\(");

	// A pattern to find a spot that warrants investigation.
	alertPat = QRegExp( "[\"'/\\w_]" );

	// multi-line commment start and end strings
	multiLineStartPat = QRegExp( "^/\\*" );
	multiLineEnd = "*/";
	
	// Collect simple rules into highlightingRules. They
	// are well distinguished by their initial character.
	HighlightingRule rule;

	// decimal and hexadecimal numeric constants
	rule.pattern = QRegExp("^\\b("
			       "\\d+(?:\\.\\d+)?(?:[eE][+-]?(\\d+))?"
			       "|"
			       "0[xX][0-9a-fA-F]+"
			       ")\\b");
	rule.format = &literalFormat;
	highlightingRules.append(rule);
	
	// String literals, both ways, and regular expression 
	rule.pattern = QRegExp("^("
			       "\"[^\"\\\\]*(\\\\.[^\"\\\\]*)*\""
			       "|"
			       "'[^'\\\\]*(\\\\.[^'\\\\]*)*'"
			       "|"
			       "/(?!/)[^/\\\\]*(\\\\.[^/\\\\]*)*/[a-z]*" // Negative lookahead!
			       ")" );
	rule.format = &literalFormat;
	highlightingRules.append(rule);

	// Line comment
	rule.pattern = QRegExp("^//[^\n]*");
	rule.format = &commentFormat;
	highlightingRules.append(rule);
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
	literalFormat.setFontWeight(QFont::Bold);
	col = Vec3f(conf->value(section + "/script_console_constant_color", defaultColor).toString()).toQColor();
	literalFormat.setForeground(col);

	// function calls
	functionFormat.setFontItalic(true);
	col = Vec3f(conf->value(section + "/script_console_function_color", defaultColor).toString()).toQColor();
	functionFormat.setForeground(col);

	// call of an unknown method
	QString unknownDefault = "1.0,0.0,0.0";
	col = Vec3f(conf->value(section + "/script_console_unknown_color", unknownDefault).toString()).toQColor();
	noMethFormat.setForeground( col );
	noMethFormat.setFontItalic( true );
}

void StelScriptSyntaxHighlighter::highlightBlock(const QString &text)
{
	int startIndex = 0;
	QSet<QString> methods = QSet<QString>();
	
	// look for closing multiline comment
	if( previousBlockState() == 1 )
	{
		startIndex = text.indexOf( multiLineEnd );
		if( startIndex == -1 )
		{
			setCurrentBlockState( 1 );
			setFormat( 0, text.length(), commentFormat );
			return;
		}
		startIndex += multiLineEnd.length();
		setFormat( 0, startIndex, commentFormat );
	}
	setCurrentBlockState( 0 );

	int mLen;
	for( int iOff = startIndex; iOff < text.length(); iOff += mLen )
	{
		// Skip to an interesting offset.
		iOff = alertPat.indexIn( text, iOff );
		if( iOff == -1 ) break;

 		mLen = 1;
        // Identifier: keyword, predefined, stellarium module or method call
		if( iOff == identPat.indexIn( text, iOff, QRegExp::CaretAtOffset ) )
		{
			QString ident = identPat.cap();
			mLen = identPat.matchedLength();
			if( keywords.contains( ident ) )
			{
				setFormat( iOff, mLen, keywordFormat );
				continue;
			}
			else if( predefineds.contains( ident ) )
			{
				setFormat( iOff, mLen, predefFormat );
				continue;
			}
			else if( mod2funcs.contains( ident ) )
			{
				methods = mod2funcs.value( ident );
				setFormat( iOff, mLen, moduleFormat );
				continue;
			}
		    if( iOff + mLen == functionPat.indexIn( text, iOff + mLen, QRegExp::CaretAtOffset ) )
			{
				if( ! methods.isEmpty() )
				{
					setFormat( iOff, mLen, methods.contains( ident ) ? methodFormat : noMethFormat );
					methods = QSet<QString>();
				} else {
					setFormat( iOff, mLen, functionFormat );
				}
				mLen += functionPat.matchedLength();
  		        continue;
			}
		}

		// Multiline comment
		if( iOff == multiLineStartPat.indexIn( text, iOff, QRegExp::CaretAtOffset ) )
		{
			mLen = multiLineStartPat.matchedLength();
			// skip to end
			startIndex = text.indexOf( multiLineEnd, iOff + mLen );
			if( startIndex == -1 )
			{
				setCurrentBlockState( 1 );
				setFormat( iOff, text.length() - iOff, commentFormat );
				return;
			}
			mLen = startIndex - iOff  + multiLineEnd.length();
			setFormat( iOff, mLen, commentFormat );
			continue;
		}

		// Straightforward tokens
		for (const HighlightingRule &rule: qAsConst(highlightingRules))
		{
			if( iOff == rule.pattern.indexIn( text, iOff, QRegExp::CaretAtOffset ) )
			{
				mLen = rule.pattern.matchedLength();
				setFormat( iOff, mLen, *(rule.format) );
				break;
			}
		}
	}
}
