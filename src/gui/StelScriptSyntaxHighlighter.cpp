/*
 * Stellarium
 * Copyright (C) 2009 Matthew Gates
 *
 * Derived from the QT syntax highlighter example under the GNU GPL.
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
#include "StelApp.hpp"
#include "StelModuleMgr.hpp"
#include "StelUtils.hpp"

#include <QtGui>
#include <QString>
#include <QColor>
#include <QSettings>
#include <QMetaMethod>

StelScriptSyntaxHighlighter::StelScriptSyntaxHighlighter(QTextDocument *parent)
	: QSyntaxHighlighter(parent)
{
	HighlightingRule rule;

	setFormats();

	// comments
	rule.pattern = QRegExp("//[^\n]*");
	rule.format = &commentFormat;
	highlightingRules.append(rule);

	// ECMAscript reserved words
	QStringList keywordPatterns;
	keywordPatterns << "\\bbreak\\b"
	                << "\\bcase\\b"
	                << "\\bcatch\\b"
	                << "\\bcontinue\\b"
	                << "\\bdefault\\b"
	                << "\\bdelete\\b"
	                << "\\bdo\\b"
	                << "\\belse\\b"
	                << "\\bfalse\\b"
	                << "\\bfinally\\b"
	                << "\\bfor\\b"
	                << "\\bfunction\\b"
	                << "\\bif\\b"
	                << "\\bin\\b"
	                << "\\binstanceof\\b"
	                << "\\bnew\\b"
	                << "\\breturn\\b"
	                << "\\bswitch\\b"
	                << "\\bthis\\b"
	                << "\\bthrow\\b"
	                << "\\btry\\b"
	                << "\\btrue\\b"
	                << "\\btypeof\\b"
	                << "\\bvar\\b"
	                << "\\bvoid\\b"
	                << "\\bundefined\\b"
	                << "\\bwhile\\b"
	                << "\\bwith\\b"
	                << "\\bArguments\\b"
	                << "\\bArray\\b"
	                << "\\bBoolean\\b"
	                << "\\bDate\\b"
	                << "\\bError\\b"
	                << "\\bEvalError\\b"
	                << "\\bFunction\\b"
	                << "\\bGlobal\\b"
	                << "\\bMath\\b"
	                << "\\bNumber\\b"
	                << "\\bObject\\b"
	                << "\\bRangeError\\b"
	                << "\\bReferenceError\\b"
	                << "\\bRegExp\\b"
	                << "\\bString\\b"
	                << "\\bSyntaxError\\b"
	                << "\\bTypeError\\b"
	                << "\\bURIError\\b";

	foreach(const QString &pattern, keywordPatterns)
	{
		rule.pattern = QRegExp(pattern);
		rule.format = &keywordFormat;
		highlightingRules.append(rule);
	}

	// highlight object names which can be used in scripting
	QStringList moduleNames;
	QStringList knownFunctionNames;
        StelModuleMgr* mmgr = &StelApp::getInstance().getModuleMgr();
        foreach (StelModule* m, mmgr->getAllModules())
        {
                moduleNames << "\\b" + m->objectName() + "\\b";

		// for each one dump known public slots
		const QMetaObject* metaObject = m->metaObject();
		for(int i = metaObject->methodOffset(); i < metaObject->methodCount(); ++i)
		{
			if (metaObject->method(i).methodType() == QMetaMethod::Slot && metaObject->method(i).access() == QMetaMethod::Public)
			{
				QString fn = metaObject->method(i).signature();
				fn.replace(QRegExp("\\(.*$"), ""); 
				fn = m->objectName() + "\\." + fn;
				knownFunctionNames << fn;
			}
		}
        }
	moduleNames << "\\bStelSkyLayerMgr\\b" << "\\bStelSkyDrawer\\b" << "\\bcore\\b";
	foreach(const QString &pattern, moduleNames)
	{
		rule.pattern = QRegExp(pattern);
		rule.format = &moduleFormat;
		highlightingRules.append(rule);
	}

	foreach(const QString &pattern, knownFunctionNames)
	{
		rule.pattern = QRegExp(pattern);
		rule.format = &moduleFormat;
		highlightingRules.append(rule);
	}

	// quoted strings
	rule.pattern = QRegExp("\".*\"");
	rule.format = &constantFormat;
	highlightingRules.append(rule);

	// decimal numeric constants
	rule.pattern = QRegExp("\\b\\d+(\\.\\d+)?\\b");
	rule.format = &constantFormat;
	highlightingRules.append(rule);

	// function calls
	rule.pattern = QRegExp("\\b[A-Za-z0-9_]+(?=\\()");
	rule.format = &functionFormat;
	highlightingRules.append(rule);

}

void StelScriptSyntaxHighlighter::setFormats(void)
{
	Vec3f col;
	QString defaultColor = "0.8,0.8,0.8";
	QSettings* conf = StelApp::getInstance().getSettings();
	QString section;

	if (StelApp::getInstance().getVisionModeNight())
		section = "night_color";
	else
		section = "color";

	// comments
	col = StelUtils::strToVec3f(conf->value(section + "/script_console_comment_color", defaultColor).toString());
	commentFormat.setForeground(QColor(col[0]*255, col[1]*255, col[2]*255));

	// ECMAscript reserved words
	col = StelUtils::strToVec3f(conf->value(section + "/script_console_keyword_color", defaultColor).toString());
	keywordFormat.setForeground(QColor(col[0]*255, col[1]*255, col[2]*255));
	keywordFormat.setFontWeight(QFont::Bold);

	// highlight object names which can be used in scripting
	moduleFormat.setFontWeight(QFont::Bold);
	col = StelUtils::strToVec3f(conf->value(section + "/script_console_module_color", defaultColor).toString());
	moduleFormat.setForeground(QColor(col[0]*255, col[1]*255, col[2]*255));

	// constants
	constantFormat.setFontWeight(QFont::Bold);
	col = StelUtils::strToVec3f(conf->value(section + "/script_console_constant_color", defaultColor).toString());
	constantFormat.setForeground(QColor(col[0]*255, col[1]*255, col[2]*255));

	// function calls
	functionFormat.setFontItalic(true);
	col = StelUtils::strToVec3f(conf->value(section + "/script_console_function_color", defaultColor).toString());
	functionFormat.setForeground(QColor(col[0]*255, col[1]*255, col[2]*255));

}

void StelScriptSyntaxHighlighter::highlightBlock(const QString &text)
{
	foreach(const HighlightingRule &rule, highlightingRules)
	{
		QRegExp expression(rule.pattern);
		int index = expression.indexIn(text);
		while (index >= 0)
		{
			int length = expression.matchedLength();
			setFormat(index, length, *(rule.format));
			index = expression.indexIn(text, index + length);
		}
	}
}

