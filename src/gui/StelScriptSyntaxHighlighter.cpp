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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "StelScriptSyntaxHighlighter.hpp"
#include "StelApp.hpp"
#include "StelModuleMgr.hpp"

#include <QtGui>

StelScriptSyntaxHighlighter::StelScriptSyntaxHighlighter(QTextDocument *parent)
	: QSyntaxHighlighter(parent)
{
	HighlightingRule rule;

	// ECMAscript reserved words
	//keywordFormat.setForeground(Qt::darkBlue);
	keywordFormat.setFontWeight(QFont::Bold);
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
		rule.format = keywordFormat;
		highlightingRules.append(rule);
	}

	// highlight object names which can be used in scripting
	moduleFormat.setFontWeight(QFont::Bold);
	moduleFormat.setForeground(QColor(0,40,0));
	QStringList moduleNames;
        StelModuleMgr* mmgr = &StelApp::getInstance().getModuleMgr();
        foreach (StelModule* m, mmgr->getAllModules())
        {
                moduleNames << "\\b" + m->objectName() + "\\b";
        }
	moduleNames << "\\bStelSkyImageMgr\\b" << "\\bStelSkyDrawer\\b" << "\\bcore\\b";
	foreach(const QString &pattern, moduleNames)
	{
		rule.pattern = QRegExp(pattern);
		rule.format = moduleFormat;
		highlightingRules.append(rule);
	}

	// comments
	singleLineCommentFormat.setForeground(Qt::darkBlue);
	rule.pattern = QRegExp("//[^\n]*");
	rule.format = singleLineCommentFormat;
	highlightingRules.append(rule);

	// quotes
	quotationFormat.setFontWeight(QFont::Bold);
	quotationFormat.setForeground(Qt::darkRed);
	rule.pattern = QRegExp("\".*\"");
	rule.format = quotationFormat;
	highlightingRules.append(rule);

	// function calls
	functionFormat.setFontItalic(true);
	//functionFormat.setForeground(Qt::darkBlue);
	rule.pattern = QRegExp("\\b[A-Za-z0-9_]+(?=\\()");
	rule.format = functionFormat;
	highlightingRules.append(rule);
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
			setFormat(index, length, rule.format);
			index = expression.indexIn(text, index + length);
		}
	}
}

