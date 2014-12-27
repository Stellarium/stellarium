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

#ifndef _STELSCRIPTSYNTAXHIGHLIGHTER_HPP_
#define _STELSCRIPTSYNTAXHIGHLIGHTER_HPP_

#include <QSyntaxHighlighter>
#include <QHash>
#include <QTextCharFormat>

class QTextDocument;

class StelScriptSyntaxHighlighter : public QSyntaxHighlighter
{
	Q_OBJECT

public:
	StelScriptSyntaxHighlighter(QTextDocument*  parent=0);
	void setFormats(void);

protected:
	void highlightBlock(const QString &text);

private:
	struct HighlightingRule
	{
		QRegExp pattern;
		QTextCharFormat* format;
	};
	QVector<HighlightingRule> highlightingRules;

	QTextCharFormat keywordFormat;
	QTextCharFormat moduleFormat;
	QTextCharFormat commentFormat;
	QTextCharFormat constantFormat;
	QTextCharFormat functionFormat;
};

#endif // _STELSCRIPTSYNTAXHIGHLIGHTER_HPP_

