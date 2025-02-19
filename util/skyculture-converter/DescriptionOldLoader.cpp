#include "DescriptionOldLoader.hpp"

#include <set>
#include <map>
#include <deque>
#include <string_view>
#include <unordered_map>
#include <QDir>
#include <QFile>
#include <QDebug>
#include <QRegularExpression>
#include <gettext-po.h>
#include "NamesOldLoader.hpp"
#include "AsterismOldLoader.hpp"
#include "ConstellationOldLoader.hpp"

namespace
{

QString stripComments(QString markdown)
{
	static const QRegularExpression commentPat("<!--.*?-->");
	markdown.replace(commentPat, "");
	return markdown;
}

QString join(const std::set<QString>& strings)
{
	QString out;
	for(const auto& str : strings)
	{
		if(out.isEmpty())
			out = str;
		else if(out.endsWith('\n'))
			out += str;
		else
			out += "\n" + str;
	}
	return out;
}

#if (QT_VERSION>=QT_VERSION_CHECK(5, 14, 0))
constexpr auto SkipEmptyParts = Qt::SkipEmptyParts;
#else
constexpr auto SkipEmptyParts = QString::SkipEmptyParts;
#endif

const QRegularExpression htmlSimpleImageRegex(R"reg(<img\b[^>/]*(?:\s+alt="([^"]+)")?\s+src="([^"]+)"(?:\s+alt="([^"]+)")?\s*/?>)reg");
const QRegularExpression htmlGeneralImageRegex(R"reg(<img\b[^>/]*\s+src="([^"]+)"[^>/]*/?>)reg");

void htmlListsToMarkdown(QString& string, const bool convertOrderedLists)
{
	// This will only handle lists whose entries don't contain HTML tags, and the
	// lists don't contain anything except <li> entries (in particular, no comments).

	static const QRegularExpression entryPattern(R"reg(<li\b\s*[^>]*>\s*([^<]+)\s*</li\s*>)reg");
	static const QRegularExpression ulistPattern(R"reg(<ul\s*>\s*(?:<li\b\s*[^>]*>[^<]+</li\s*>\s*)+</ul\s*>)reg");
	static const QRegularExpression outerUListTagPattern(R"reg(</?ul\s*>)reg");
	for(auto matches = ulistPattern.globalMatch(string); matches.hasNext(); )
	{
		const auto& match = matches.next();
		auto list = match.captured(0);
		list.replace(outerUListTagPattern, "\n");
		list.replace(entryPattern, "\n - \\1\n");
		string.replace(match.captured(0), list);
	}

	if(convertOrderedLists)
	{
		static const QRegularExpression olistPattern(R"reg(<ol\s*>\s*(?:<li\b\s*[^>]*>[^<]+</li\s*>\s*)+</ol\s*>)reg");
		static const QRegularExpression outerOListTagPattern(R"reg(</?ol\s*>)reg");
		for(auto matches = olistPattern.globalMatch(string); matches.hasNext(); )
		{
			const auto& match = matches.next();
			auto list = match.captured(0);
			list.replace(outerOListTagPattern, "\n");
			list.replace(entryPattern, "\n 1. \\1\n");
			string.replace(match.captured(0), list);
		}
	}
}

void htmlBlockQuoteToMarkdown(QString& string)
{
	static const QRegularExpression blockquotePattern(R"reg(<blockquote\s*>\s*[^<]*</blockquote\s*>)reg");
	static const QRegularExpression outerBQTagPattern(R"reg(\s*</?blockquote\s*>\s*)reg");
	static const QRegularExpression emptyFinalLines(R"reg((?:\n> *)+\n*$)reg");
	for(auto matches = blockquotePattern.globalMatch(string); matches.hasNext(); )
	{
		const auto& match = matches.next();
		auto blockquote = match.captured(0).trimmed();
		blockquote.replace(outerBQTagPattern, "\n");
		blockquote.replace("\n", "\n> ");
		blockquote.replace(emptyFinalLines, "\n");
		blockquote = "\n" + blockquote + "\n";
		string.replace(match.captured(0), blockquote);
	}
}

void formatDescriptionLists(QString& string)
{
	// We don't convert DLs into Markdown (since there's no such
	// concept there), but we want them to be editor-friendly.
	string.replace(QRegularExpression(" *(<dl\\s*>)\\s*(<dt\\s*>)"), "\\1\n \\2");
	string.replace(QRegularExpression("( *<dt\\s*>)"), " \\1");
//	string.replace(QRegularExpression("(<dd\\s*>)"), "\n\t\\1");
	string.replace(QRegularExpression("(</dd\\s*>)"), "\\1\n");
	string.replace(QRegularExpression("(</dd\\s*>)\n </dl>"), "\\1\n</dl>");
}

void htmlTablesToMarkdown(QString& string)
{
	// Using a single regex to find all tables without merging them into
	// one capture appears to be too hard. Let's go in a lower-level way,
	// by finding all the beginnings and ends manually.
	const QRegularExpression tableBorderPattern(R"reg((<table\b[^>]*>)|(</table\s*>))reg");
	bool foundStart = false;
	int startPos = -1, tagStartPos = -1;
	QStringList tables;
	std::vector<std::pair<int,int>> tablesPositions;
	bool isLayoutTable = false;
	for(auto matches = tableBorderPattern.globalMatch(string); matches.hasNext(); )
	{
		const auto& match = matches.next();
		const auto& startCap = match.captured(1);
		const auto& endCap = match.captured(2);
		if(!startCap.isEmpty() && endCap.isEmpty() && !foundStart)
		{
			foundStart = true;
			tagStartPos = match.capturedStart(1);
			startPos = match.capturedEnd(1);
			isLayoutTable = startCap.contains("class=\"layout\"");
		}
		else if(startCap.isEmpty() && !endCap.isEmpty() && foundStart)
		{
			foundStart = false;
			Q_ASSERT(startPos >= 0);
			Q_ASSERT(tagStartPos >= 0);
			const auto endPos = match.capturedStart(2);
			const auto tagEndPos = match.capturedEnd(2);
			if(!isLayoutTable)
			{
				tables += string.mid(startPos, endPos - startPos);
				tablesPositions.emplace_back(tagStartPos, tagEndPos);
			}
		}
		else
		{
			qWarning() << "Inconsistency between table start and end tags detected, can't process tables further";
			return;
		}
	}

	// Now do the actual conversion
	for(int n = 0; n < tables.size(); ++n)
	{
		const auto& table = tables[n];
		if(table.contains(QRegularExpression("\\s(?:col|row)span=")))
		{
			qWarning() << "Row/column spans are not supported, leaving the table in HTML form";
			continue;
		}
		if(!table.contains(QRegularExpression("^\\s*<tr\\s*>")))
		{
			qWarning().noquote() << "Unexpected table contents (expected it to start with <tr>), keeping the table in HTML form. Table:\n" << table;
			continue;
		}
		if(!table.contains(QRegularExpression("</tr\\s*>\\s*$")))
		{
			qWarning().noquote() << "Unexpected table contents (expected it to end with </tr>), keeping the table in HTML form. Table:\n" << table;
			continue;
		}
		auto rows = table.split(QRegularExpression("\\s*</tr\\s*>\\s*"), SkipEmptyParts);
		// The closing row tags have been removed by QString::split, now remove the opening tags
		static const QRegularExpression trOpenTag("^\\s*<tr\\s*>\\s*");
		for(auto& row : rows) row.replace(trOpenTag, "");

		QString markdownTable;
		// Now convert the rows
		for(const auto& row : rows)
		{
			if(row.simplified().isEmpty()) continue;
			if(!row.contains(QRegularExpression("^\\s*<t[dh]\\s*>")))
			{
				qWarning() << "Unexpected row contents (expected it to start with <td> or <th>), keeping the table in HTML form. Row:" << row;
				goto nextTable;
			}
			if(!row.contains(QRegularExpression("</t[dh]\\s*>\\s*$")))
			{
				qWarning() << "Unexpected row contents (expected it to end with </td> or </th>), keeping the table in HTML form. Row:" << row;
				goto nextTable;
			}
			auto cols = row.split(QRegularExpression("\\s*</t[dh]\\s*>\\s*"), SkipEmptyParts);
			// The closing column tags have been removed by QString::split, now remove the opening tags
			static const QRegularExpression tdOpenTag("^\\s*<t[dh]\\s*>\\s*");
			for(auto& col : cols) col.replace(tdOpenTag, "");

			// Finally, emit the rows
			const bool firstRow = markdownTable.isEmpty();
			if(firstRow) markdownTable += "\n"; // make sure the table starts as a new paragraph
			markdownTable += "|";
			for(const auto& col : cols)
			{
				if(col.isEmpty())
					markdownTable += "   ";
				else
					markdownTable += col;
				markdownTable += '|';
			}
			markdownTable += '\n';
			if(firstRow)
			{
				markdownTable += '|';
				for(const auto& col : cols)
				{
					markdownTable += QString(std::max(3, int(col.size())), QChar('-'));
					markdownTable += '|';
				}
				markdownTable += "\n";
			}
		}

		// Replace the HTML table with the newly-created Markdown one
		{
			const auto lengthToReplace = tablesPositions[n].second - tablesPositions[n].first;
			string.replace(tablesPositions[n].first, lengthToReplace, markdownTable);
			// Fixup the positions of the subsequent tables
			const int delta = markdownTable.size() - lengthToReplace;
			for(auto& positions : tablesPositions)
			{
				positions.first += delta;
				positions.second += delta;
			}
		}

nextTable:
		continue;
	}

	// Format the tables that we've failed to convert with each row
	// on its line, and each column entry on an indented line.
	string.replace(QRegularExpression("(<tr(?:\\s+[^>]*)*>)"), "\n\\1");
	string.replace(QRegularExpression("(</tr\\s*>)"), "\n\\1");
	string.replace(QRegularExpression("(<td(?:\\s+[^>]*)*>)"), "\n\t\\1");
	string.replace(QRegularExpression("(</table\\s*>)"), "\n\\1");
}

QString readReferencesFile(const QString& inDir)
{
	const auto path = inDir + "/reference.fab";
	if (path.isEmpty())
	{
		qWarning() << "Reference file wasn't found";
		return "";
	}
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qWarning() << "WARNING - could not open" << QDir::toNativeSeparators(path);
		return "";
	}
	QString record;
	// Allow empty and comment lines where first char (after optional blanks) is #
	static const QRegularExpression commentRx("^(\\s*#.*|\\s*)$");
	QString reference = "## References\n\n";
	int totalRecords=0;
	int readOk=0;
	int lineNumber=0;
	while(!file.atEnd())
	{
		record = QString::fromUtf8(file.readLine()).trimmed();
		lineNumber++;
		if (commentRx.match(record).hasMatch())
			continue;

		totalRecords++;
		static const QRegularExpression refRx("\\|");
		#if (QT_VERSION>=QT_VERSION_CHECK(5, 14, 0))
		QStringList ref = record.split(refRx, Qt::KeepEmptyParts);
		#else
		QStringList ref = record.split(refRx, QString::KeepEmptyParts);
		#endif
		// 1 - URID; 2 - Reference; 3 - URL (optional)
		if (ref.count()<2)
			qWarning() << "Error: cannot parse record at line" << lineNumber << "in references file" << QDir::toNativeSeparators(path);
		else if (ref.count()<3)
		{
			qWarning() << "Warning: record at line" << lineNumber << "in references file"
			           << QDir::toNativeSeparators(path) << " has wrong format (RefID: " << ref.at(0) << ")! Let's use fallback mode...";
			reference.append(QString(" - [#%1]: %2\n").arg(ref[0], ref[1]));
			readOk++;
		}
		else
		{
			if (ref.at(2).isEmpty())
				reference.append(QString(" - [#%1]: %2\n").arg(ref[0], ref[1]));
			else
				reference.append(QString(" - [#%1]: [%2](%3)\n").arg(ref[0], ref[1], ref[2]));
			readOk++;
		}
	}
	if(readOk != totalRecords)
		qDebug() << "Loaded" << readOk << "/" << totalRecords << "references";

	return reference;
}

void cleanupWhitespace(QString& markdown)
{
	// Clean too long chains of newlines
	markdown.replace(QRegularExpression("\n[ \t]*\n[ \t]*\n+"), "\n\n");
	// Same for such chains inside blockquotes
	markdown.replace(QRegularExpression("\n>[ \t]*(?:\n>[ \t]*)+\n"), "\n>\n");

	// Remove trailing spaces
	markdown.replace(QRegularExpression("[ \t]+\n"), "\n");

	// Make unordered lists a bit denser
	const QRegularExpression ulistSpaceListPattern("(\n -[^\n]+)\n+(\n \\-)");
	//  1. Remove space between odd and even entries
	markdown.replace(ulistSpaceListPattern, "\\1\\2");
	//  2. Remove space between even and odd entries (same replacement rule)
	markdown.replace(ulistSpaceListPattern, "\\1\\2");

	// Make ordered lists a bit denser
	const QRegularExpression olistSpaceListPattern("(\n 1\\.[^\n]+)\n+(\n 1)");
	//  1. Remove space between odd and even entries
	markdown.replace(olistSpaceListPattern, "\\1\\2");
	//  2. Remove space between even and odd entries (same replacement rule)
	markdown.replace(olistSpaceListPattern, "\\1\\2");

	const bool startsWithList = markdown.startsWith(" 1. ") || markdown.startsWith(" - ");
	markdown = (startsWithList ? " " : "") + markdown.trimmed() + "\n";
}

[[nodiscard]] QString convertHTMLToMarkdown(const QString& html, const bool fullerConversionToMarkdown,
                                            const bool footnotesToRefs, const bool convertOrderedLists)
{
	QString markdown = html;

	if(fullerConversionToMarkdown)
	{
		markdown.replace(QRegularExpression(R"reg(<\s*html\s+dir="[^"]+"\s*>|</html \s>)reg"), "");
		markdown.replace(QRegularExpression("[\n\t ]+"), " ");
	}
	else
	{
		// Twice to handle even/odd cases
		markdown.replace(QRegularExpression("\n[ \t]*\n"), "\n");
		markdown.replace(QRegularExpression("\n[ \t]*\n"), "\n");
	}

	// Replace <notr> and </notr> tags with placeholders that don't
	// look like tags, so as not to confuse the replacements below.
	const QString notrOpenPlaceholder = "{22c35d6a-5ec3-4405-aeff-e79998dc95f7}";
	const QString notrClosePlaceholder = "{2543be41-c785-4283-a4cf-ce5471d2c422}";
	markdown.replace(QRegularExpression("<notr\\s*>"), notrOpenPlaceholder);
	markdown.replace(QRegularExpression("</notr\\s*>"), notrClosePlaceholder);

	// Same for <sup> and </sup>.
	const QString supOpenPlaceholder = "{4edbb3ef-6a33-472a-8faf-1b006dda557c}";
	const QString supClosePlaceholder = "{e4e12021-9fdf-48de-80ef-e65f0c42738f}";
	const auto supOpenPlaceholderPattern = "\\" + supOpenPlaceholder;
	const auto supClosePlaceholderPattern = "\\" + supClosePlaceholder;
	markdown.replace(QRegularExpression("<sup\\s*>"), supOpenPlaceholder);
	markdown.replace(QRegularExpression("</sup\\s*>"), supClosePlaceholder);

	if(fullerConversionToMarkdown)
	{
		// Replace HTML line breaks with the Markdown ones
		markdown.replace(QRegularExpression("<br\\s*/?>"), "\n\n");

		const auto replaceEmpasis = [&markdown] {
			// Replace simple HTML emphases with the Markdown ones
			markdown.replace(QRegularExpression("<i>(\\s*)([^<\\s]{1,2}|[^<\\s][^<]+[^<\\s])(\\s*)</i>"), "\\1*\\2*\\3");
			markdown.replace(QRegularExpression("<em>(\\s*)([^<\\s]{1,2}|[^<\\s][^<]+[^<\\s])(\\s*)</em>"), "\\1*\\2*\\3");
			markdown.replace(QRegularExpression("<b>(\\s*)([^<\\s]{1,2}|[^<\\s][^<]+[^<\\s])(\\s*)</b>"), "\\1**\\2**\\3");
			markdown.replace(QRegularExpression("<strong>(\\s*)([^<\\s]{1,2}|[^<\\s][^<]+[^<\\s])(\\s*)</strong>"), "\\1**\\2**\\3");
		};
		replaceEmpasis();

		// Replace simple HTML images with the Markdown ones
		markdown.replace(htmlSimpleImageRegex, R"rep(![\1\3](\2))rep");

		if(footnotesToRefs)
		{
			// Hyperlinks to footnotes
			markdown.replace(QRegularExpression(supOpenPlaceholderPattern+
			                                     R"regex(\s*<a\s+href="#footnote-([0-9]+)"\s*>\s*\[[^\]]+\]([,\s]*)</a\s*>\s*)regex"+
			                                     supClosePlaceholderPattern,
			                                    QRegularExpression::DotMatchesEverythingOption), "[#\\1]\\2");
		}

		// Replace simple HTML hyperlinks with the Markdown ones
		//  older version (do we want it?): markdown.replace(QRegularExpression("([^>])<a\\s+href=\"([^\"]+)\"(?:\\s[^>]*)?>([^<]+)</a\\s*>([^<])"), "\\1[\\3](\\2)\\4");
		markdown.replace(QRegularExpression("<a\\s+href=\"([^\"]+)\"(?:\\s[^>]*)?>([^<]+)</a\\s*>"), "[\\2](\\1)");

		if(footnotesToRefs)
		{
			// First footnote (to prepend <ul> before it)
			markdown.replace(QRegularExpression(     R"regex(<p\s*id="footnote-(1)"\s*>(?:\[[^\]]+\] *)?([^<]*)</p\s*>)regex",
			                                    QRegularExpression::DotMatchesEverythingOption), "<ul>\n <li>[#\\1]: \\2</li>\n");
			// Last footnote (to append </ul> after it)
			markdown.replace(QRegularExpression(R"regex(<p\s*id="footnote-([0-9]+)"\s*>(?:\[[^\]]+\] *)?([^<]*)</p\s*>\s*($|<h))regex",
			                                    QRegularExpression::DotMatchesEverythingOption), " <li>[#\\1]: \\2</li>\n</ul>\n\\3");
			// Middle footnotes
			markdown.replace(QRegularExpression(R"regex(<p\s*id="footnote-([0-9]+)"\s*>(?:\[[^\]]+\] *)?([^<]*)</p\s*>)regex",
			                                    QRegularExpression::DotMatchesEverythingOption), " <li>[#\\1]: \\2</li>\n");
		}

		// Retry italics etc. This might now work after the above conversions if it hasn't worked before.
		replaceEmpasis();

		// Replace HTML paragraphs with the Markdown ones
		markdown.replace(QRegularExpression("<p(?:\\s+[^>]*)*>([^<]+)</p>"), "\n\\1\n");
	}

	// Replace simple HTML headings with corresponding Markdown ones
	for(int n = 1; n <= 6; ++n)
		markdown.replace(QRegularExpression(QString("<h%1(?:\\s+[^>]*)*>([^<]+)</h%1> *").arg(n)), "\n" + QString(n, QChar('#'))+" \\1\n");

	if(fullerConversionToMarkdown)
	{
		htmlTablesToMarkdown(markdown);

		formatDescriptionLists(markdown);
	}

	htmlListsToMarkdown(markdown, convertOrderedLists);
	htmlBlockQuoteToMarkdown(markdown);

	if(fullerConversionToMarkdown)
	{
		// Retry paragraphs. This might now work after the above conversions if it hasn't worked before.
		markdown.replace(QRegularExpression("<p(?:\\s+[^>]*)*>([^<]+)</p>"), "\n\\1\n");
	}
	else
	{
		markdown.replace(QRegularExpression("<p\\s*>([^<]+)</p>"), "\n\\1\n");
	}

	cleanupWhitespace(markdown);

	// Restore the reserved tags
	markdown.replace(notrOpenPlaceholder,  "<notr>");
	markdown.replace(notrClosePlaceholder, "</notr>");
	markdown.replace(supOpenPlaceholder,  "<sup>");
	markdown.replace(supClosePlaceholder, "</sup>");

	return markdown;
}

void addMissingTextToMarkdown(QString& markdown, const QString& inDir, const QString& author, const QString& credit, const QString& license)
{
	// Add missing "Introduction" heading if we have a headingless intro text
	if(!markdown.contains(QRegularExpression("^\\s*# [^\n]+\n+\\s*##\\s*Introduction\n")))
		markdown.replace(QRegularExpression("^(\\s*# [^\n]+\n+)(\\s*[^#])"), "\\1## Introduction\n\n\\2");
	if(!markdown.contains("\n## Description\n"))
	   markdown.replace(QRegularExpression("(\n## Introduction\n[^#]+\n)(\\s*#)"), "\\1## Description\n\n\\2");

	// Add some sections the info for which is contained in info.ini in the old format
	if(markdown.contains(QRegularExpression("\n##\\s+(?:References|External\\s+links)\\s*\n")))
		markdown.replace(QRegularExpression("(\n##[ \t]+)External[ \t]+links([ \t]*\n)"), "\\1References\\2");
	auto referencesFromFile = readReferencesFile(inDir);

	if(markdown.contains(QRegularExpression("\n##\\s+Authors?\\s*\n")))
	{
		qWarning() << "Authors section already exists, not adding the authors from info.ini";

		// But do add references before this section
		if(!referencesFromFile.isEmpty())
			markdown.replace(QRegularExpression("(\n##\\s+Authors?\\s*\n)"), "\n"+referencesFromFile + "\n\\1");
	}
	else
	{
		// First add references
		if(!referencesFromFile.isEmpty())
			markdown += referencesFromFile + "\n";

		if(credit.isEmpty())
			markdown += QString("\n## Authors\n\n%1\n").arg(author);
		else
			markdown += "\n## Authors\n\nAuthor is " + author + ". Additional credit goes to " + credit + "\n";
	}

	if(markdown.contains(QRegularExpression("\n##\\s+License\\s*\n")))
		qWarning() << "License section already exists, not adding the license from info.ini";
	else
		markdown += "\n## License\n\n" + license + "\n";

	cleanupWhitespace(markdown);
}

struct Section
{
	int level = -1;
	int levelAddition = 0;
	int headerLineStartPos = -1;
	int headerStartPos = -1; // including #..#
	int bodyStartPos = -1;
	QString title;
	QString body;
	std::deque<int> subsections;
};

std::vector<Section> splitToSections(const QString& markdown)
{
	const QRegularExpression sectionHeaderPattern("^[ \t]*((#+)\\s+(.*[^\\s])\\s*)$", QRegularExpression::MultilineOption);
	std::vector<Section> sections;
	for(auto matches = sectionHeaderPattern.globalMatch(markdown); matches.hasNext(); )
	{
		sections.push_back({});
		auto& section = sections.back();
		const auto& match = matches.next();
		section.headerLineStartPos = match.capturedStart(0);
		section.headerStartPos = match.capturedStart(1);
		section.level = match.captured(2).length();
		section.title = match.captured(3);
		section.bodyStartPos = match.capturedEnd(0) + 1/*\n*/;

		if(section.title.trimmed() == "Author")
			section.title = "Authors";
	}

	for(unsigned n = 0; n < sections.size(); ++n)
	{
		if(n+1 < sections.size())
			sections[n].body = markdown.mid(sections[n].bodyStartPos,
			                                std::max(0, sections[n+1].headerLineStartPos - sections[n].bodyStartPos))
			                           .replace(QRegularExpression("^\n*|\\s*$"), "");
		else
			sections[n].body = markdown.mid(sections[n].bodyStartPos).replace(QRegularExpression("^\n*|\\s*$"), "");
	}

	return sections;
}

bool isStandardTitle(const QString& title)
{
	return title == "Introduction" ||
	       title == "Description" ||
	       title == "Constellations" ||
	       title == "References" ||
	       title == "Authors" ||
	       title == "License";
}

void gettextpo_xerror(int severity, po_message_t message, const char *filename, size_t lineno, size_t column, int multiline_p, const char *message_text)
{
	(void)message;
	qWarning().nospace() << "libgettextpo: " << filename << ":" << lineno << ":" << column << ": " << (multiline_p ? "\n" : "") << message_text;
	if(severity == PO_SEVERITY_FATAL_ERROR)
		std::abort();
}

void gettextpo_xerror2(int severity,
                       po_message_t message1, const char *filename1, size_t lineno1, size_t column1, int multiline_p1, const char *message_text1,
                       po_message_t message2, const char *filename2, size_t lineno2, size_t column2, int multiline_p2, const char *message_text2)
{
	(void)message1;
	(void)message2;
	qWarning().nospace() << "libgettextpo: error with two messages:";
	qWarning().nospace() << "libgettextpo: message 1 error: " << filename1 << ":" << lineno1 << ":" << column1 << ": " << (multiline_p1 ? "\n" : "") << message_text1;
	qWarning().nospace() << "libgettextpo: message 2 error: " << filename2 << ":" << lineno2 << ":" << column2 << ": " << (multiline_p2 ? "\n" : "") << message_text2;
	if(severity == PO_SEVERITY_FATAL_ERROR)
		std::abort();
}
}

QString DescriptionOldLoader::translateSection(const QString& markdown, const qsizetype bodyStartPos,
                                               const qsizetype bodyEndPos, const QString& locale, const QString& sectionName) const
{
	auto text = markdown.mid(bodyStartPos, bodyEndPos - bodyStartPos);
	text.replace(QRegularExpression("^\n*|\n*$"), "");
	for(const auto& entry : translations[locale])
	{
		if(entry.english == text)
		{
			text = stripComments(entry.translated);
			break;
		}
		if(entry.comment.find(QString("Sky culture %1 section in markdown format").arg(sectionName.trimmed().toLower())) != entry.comment.end())
		{
			qWarning() << " *** BAD TRANSLATION ENTRY for section" << sectionName << "in locale" << locale;
			qWarning() << "  Entry msgid:" << entry.english;
			qWarning() << "  English section text:" << text << "\n";
		}
	}
	return text;
}

QString DescriptionOldLoader::translateDescription(const QString& markdownInput, const QString& locale) const
{
	const auto markdown = stripComments(markdownInput);

	const QRegularExpression headerPat("^# +(.+)$", QRegularExpression::MultilineOption);
	const auto match = headerPat.match(markdown);
	QString name;
	if (match.isValid())
	{
		name = match.captured(1);
	}
	else
	{
		qCritical().nospace() << "Failed to get sky culture name: got " << match.lastCapturedIndex() << " matches instead of 1";
		name = "Unknown";
	}

	QString text = "# " + name + "\n\n";
	const QRegularExpression sectionNamePat("^## +(.+)$", QRegularExpression::MultilineOption);
	QString prevSectionName;
	qsizetype prevBodyStartPos = -1;
	for (auto it = sectionNamePat.globalMatch(markdown); it.hasNext(); )
	{
		const auto match = it.next();
		const auto sectionName = match.captured(1);
		const auto nameStartPos = match.capturedStart(0);
		const auto bodyStartPos = match.capturedEnd(0);
		if (!prevSectionName.isEmpty())
		{
			const auto sectionText = translateSection(markdown, prevBodyStartPos, nameStartPos, locale, prevSectionName);
			text += "## " + prevSectionName + "\n\n";
			if (!sectionText.isEmpty())
				text += sectionText + "\n\n";
		}
		prevBodyStartPos = bodyStartPos;
		prevSectionName = sectionName;
	}
	if (prevBodyStartPos >= 0)
	{
		const auto sectionText = translateSection(markdown, prevBodyStartPos, markdown.size(), locale, prevSectionName);
		if (!sectionText.isEmpty())
		{
			text += "## " + prevSectionName + "\n\n";
			text += sectionText;
		}
	}

	return text;
}
void DescriptionOldLoader::loadTranslationsOfNames(const QString& poBaseDir, const QString& cultureIdQS, const QString& englishName,
                                                   const ConstellationOldLoader& consLoader, const AsterismOldLoader& astLoader,
                                                   const NamesOldLoader& namesLoader)
{
	po_xerror_handler handler = {gettextpo_xerror, gettextpo_xerror2};
	const auto cultureId = cultureIdQS.toStdString();

	const auto poDir = poBaseDir+"/stellarium-skycultures";
	for(const auto& fileName : QDir(poDir).entryList({"*.po"}))
	{
		const QString locale = fileName.chopped(3);
		const auto file = po_file_read((poDir+"/"+fileName).toStdString().c_str(), &handler);
		if(!file) continue;

		const auto header = po_file_domain_header(file, nullptr);
		if(header) poHeaders[locale] = header;

		qDebug().nospace() << "Processing translations of names for locale " << locale << "...";
		auto& dict = translations[locale];
		std::unordered_map<QString/*msgid*/,int/*position*/> insertedNames;

		// First try to find translation for the name of the sky culture
		bool scNameTranslated = false;
		if(const auto scNameFile = po_file_read((poBaseDir+"/stellarium/"+fileName).toStdString().c_str(), &handler))
		{
			const auto domains = po_file_domains(scNameFile);
			for(auto domainp = domains; *domainp && !scNameTranslated; domainp++)
			{
				const auto domain = *domainp;
				po_message_iterator_t iterator = po_message_iterator(scNameFile, domain);

				for(auto message = po_next_message(iterator); message != nullptr; message = po_next_message(iterator))
				{
					const auto msgid = po_message_msgid(message);
					const auto msgstr = po_message_msgstr(message);
					const auto ctxt = po_message_msgctxt(message);
					if(ctxt && ctxt == std::string_view("sky culture") && msgid == englishName)
					{
						dict.insert(dict.begin(), {{"Sky culture name"}, msgid, msgstr});
						scNameTranslated = true;
						break;
					}
				}
				po_message_iterator_free(iterator);
			}
			po_file_free(scNameFile);
		}

		if(!scNameTranslated)
			qWarning() << "Couldn't find a translation for the name of the sky culture";

		const std::map<std::string, std::string_view> sourceFiles{
			{"skycultures/"+cultureId+"/star_names.fab", "star"},
			{"skycultures/"+cultureId+"/dso_names.fab", "dso"},
			{"skycultures/"+cultureId+"/planet_names.fab", "planet"},
			{"skycultures/"+cultureId+"/asterism_names.eng.fab", "asterism"},
			{"skycultures/"+cultureId+"/constellation_names.eng.fab", "constellation"},
		};
		QString comments;
		const auto domains = po_file_domains(file);
		for(auto domainp = domains; *domainp; domainp++)
		{
			const auto domain = *domainp;
			po_message_iterator_t iterator = po_message_iterator(file, domain);

			for(auto message = po_next_message(iterator); message != nullptr; message = po_next_message(iterator))
			{
				const auto msgid = po_message_msgid(message);
				const auto msgstr = po_message_msgstr(message);
				QString comments = po_message_extracted_comments(message);
				for(int n = 0; ; ++n)
				{
					const auto filepos = po_message_filepos(message, n);
					if(!filepos) break;
					const auto refFileName = po_filepos_file(filepos);
					const auto ref = sourceFiles.find(refFileName);
					if(ref == sourceFiles.end()) continue;
					const auto type = ref->second;
					if(type == "constellation")
					{
						const auto cons = consLoader.find(msgid);
						if(cons)
						{
							comments = englishName+" constellation";
							if(!cons->nativeName.isEmpty())
								comments += ", native: "+cons->nativeName;
							comments += '\n' + cons->translatorsComments;
						}
						else
						{
							continue;
						}
					}
					else if(type == "asterism")
					{
						if(const auto aster = astLoader.find(msgid))
						{
							comments = englishName+" asterism";
							comments += '\n' + aster->getTranslatorsComments();
						}
						else
						{
							continue;
						}
					}
					else if(type == "star")
					{
						const auto star = namesLoader.findStar(msgid);
						if(star && star->HIP > 0)
						{
							if(star->nativeName.isEmpty())
								comments = QString("%1 name for HIP %2").arg(englishName).arg(star->HIP);
							else
								comments = QString("%1 name for HIP %2, native: %3").arg(englishName).arg(star->HIP).arg(star->nativeName);
							comments += '\n' + star->translatorsComments;
						}
						else
						{
							continue;
						}
					}
					else if(type == "planet")
					{
						if(const auto planet = namesLoader.findPlanet(msgid))
						{
							if(planet->native.isEmpty())
								comments = QString("%1 name for NAME %2").arg(englishName).arg(planet->id);
							else
								comments = QString("%1 name for NAME %2, native: %3").arg(englishName).arg(planet->id, planet->native);
							comments += '\n' + planet->translatorsComments;
						}
						else
						{
							continue;
						}
					}
					else if(type == "dso")
					{
						if(const auto dso = namesLoader.findDSO(msgid))
						{
							if(dso->nativeName.isEmpty())
								comments = QString("%1 name for %2").arg(englishName).arg(dso->id);
							else
								comments = QString("%1 name for %2, native: %3").arg(englishName).arg(dso->id, dso->nativeName);
							comments += '\n' + dso->translatorsComments;
						}
						else
						{
							continue;
						}
					}
					if(const auto it = insertedNames.find(msgid); it != insertedNames.end())
					{
						auto& entry = dict[it->second];
						entry.comment.insert(comments);
						continue;
					}
					insertedNames[msgid] = dict.size();
					dict.push_back({{comments}, msgid, msgstr});
				}
			}
			po_message_iterator_free(iterator);
		}
		po_file_free(file);
	}
}

void DescriptionOldLoader::locateAndRelocateAllInlineImages(QString& html, const bool saveToRefs)
{
	for(auto matches = htmlGeneralImageRegex.globalMatch(html); matches.hasNext(); )
	{
		const auto& match = matches.next();
		auto path = match.captured(1);
		auto updatedPath = path;
		if(!path.startsWith("illustrations/"))
		{
			updatedPath = "illustrations/" + path;
			const auto imgTag = match.captured(0);
			const auto updatedImgTag = QString(imgTag).replace(path, updatedPath);
			html.replace(imgTag, updatedImgTag);
		}
		if(saveToRefs)
			imageHRefs.emplace_back(path, updatedPath);
	}
}

void DescriptionOldLoader::load(const QString& inDir, const QString& poBaseDir, const QString& cultureId, const QString& englishName,
                                const QString& author, const QString& credit, const QString& license,
                                const ConstellationOldLoader& consLoader, const AsterismOldLoader& astLoader, const NamesOldLoader& namesLoader,
                                const bool fullerConversionToMarkdown, const bool footnotesToRefs, const bool convertOrderedLists,
                                const bool genTranslatedMD)
{
	inputDir = inDir;
	const auto englishDescrPath = inDir+"/description.en.utf8";
	QFile englishDescrFile(englishDescrPath);
	if(!englishDescrFile.open(QFile::ReadOnly))
	{
		qCritical().noquote() << "Failed to open file" << englishDescrPath;
		return;
	}
	QString html = englishDescrFile.readAll();
	locateAndRelocateAllInlineImages(html, true);
	qDebug() << "Processing English description...";
	markdown = convertHTMLToMarkdown(html, fullerConversionToMarkdown, footnotesToRefs, convertOrderedLists);

	auto englishSections = splitToSections(markdown);
	const int level1sectionCount = std::count_if(englishSections.begin(), englishSections.end(),
	                                             [](auto& s){return s.level==1;});
	if(level1sectionCount != 1)
	{
		qCritical().nospace() << "Unexpected number of level-1 sections in file " << englishDescrPath
		                      << " (expected 1, found " << level1sectionCount
		                      << "), will not convert the description";
		return;
	}

	// Mark all sections with level>2 to be subsections of the nearest preceding level<=2 sections
	std::deque<int> subsections;
	for(int n = signed(englishSections.size()) - 1; n >= 0; --n)
	{
		const bool hasStandardTitle = isStandardTitle(englishSections[n].title);
		if(hasStandardTitle && englishSections[n].level != 2)
		{
			qWarning() << "Warning: found a section titled" << englishSections[n].title
			           << "but having level" << englishSections[n].level << " instead of 2";
		}

		if(englishSections[n].level > 2 || (englishSections[n].level == 2 && !hasStandardTitle))
		{
			subsections.push_front(n);
		}
		else
		{
			englishSections[n].subsections = std::move(subsections);
			subsections.clear();
		}
	}

	// Increase the level of all level-2 sections and their subsections unless they have one of the standard titles
	for(auto& section : englishSections)
	{
		if(section.level != 2 || isStandardTitle(section.title)) continue;
		if(section.level == 2)
		{
			for(const int n : section.subsections)
				englishSections[n].levelAddition = 1;
		}
		section.levelAddition = 1;
	}

	if(englishSections.empty())
	{
		qCritical() << "No sections found in" << englishDescrPath;
		return;
	}

	if(englishSections[0].level != 1)
	{
		qCritical() << "Unexpected section structure: first section must have level 1, but instead has" << englishSections[0].level;
		return;
	}

	if(englishSections[0].title.trimmed().toLower() != englishName.toLower())
	{
		qWarning().nospace() << "English description caption is not the same as the name of the sky culture: "
		                     << englishSections[0].title << " vs " << englishName << ". Will change the caption to match the name.";
		englishSections[0].title = englishName;
	}

	const QRegularExpression localePattern("description\\.([^.]+)\\.utf8");

	// This will contain the final form of the English sections for use as a key
	// in translations as well as to reconstruct the main description.md
	std::vector<std::pair<QString/*title*/,QString/*section*/>> finalEnglishSections;
	bool finalEnglishSectionsDone = false;

	bool descrSectionExists = false;
	for(const auto& section : englishSections)
	{
		if(section.level + section.levelAddition != 2)
			continue;
		if(section.title.trimmed().toLower() == "description")
			descrSectionExists = true;
	}

	std::vector<QString> locales;
	for(const auto& fileName : QDir(inDir).entryList({"description.*.utf8"}))
	{
		if(fileName == "description.en.utf8") continue;

		const auto localeMatch = localePattern.match(fileName);
		if(!localeMatch.isValid())
		{
			qCritical() << "Failed to extract locale from file name" << fileName;
			continue;
		}
		const auto locale = localeMatch.captured(1);
		locales.push_back(locale);
		const auto path = inDir + "/" + fileName;
		QFile file(path);
		if(!file.open(QFile::ReadOnly))
		{
			qCritical().noquote() << "Failed to open file" << path << "\n";
			continue;
		}
		qDebug().nospace() << "Processing description for locale " << locale << "...";
		QString localizedHTML = file.readAll();
		locateAndRelocateAllInlineImages(localizedHTML, false);
		auto trMD0 = convertHTMLToMarkdown(localizedHTML, fullerConversionToMarkdown, footnotesToRefs, convertOrderedLists);
		const auto translationMD = trMD0.replace(QRegularExpression("<notr>([^<]+)</notr>"), "\\1");
		const auto translatedSections = splitToSections(translationMD);
		if(translatedSections.size() != englishSections.size())
		{
			qCritical().nospace().noquote() << "Number of sections (" << translatedSections.size()
			                                << ") in description for locale " << locale
			                                << " doesn't match that of the English description ("
			                                << englishSections.size() << "). Skipping this translation.";
			auto dbg = qDebug().nospace().noquote();
			dbg << " ** English section titles:\n";
			for(const auto& sec : englishSections)
				dbg << sec.level << ": " << sec.title << "\n";
			dbg << " ** Translated section titles:\n";
			for(const auto& sec : translatedSections)
				dbg << sec.level << ": " << sec.title << "\n";
			dbg << "\n____________________________________________\n";
			continue;
		}

		bool sectionsOK = true;
		for(unsigned n = 0; n < englishSections.size(); ++n)
		{
			if(translatedSections[n].level != englishSections[n].level)
			{
				qCritical() << "Section structure of English text and translation for"
				            << locale << "doesn't match, skipping this translation";
				auto dbg = qDebug().nospace();
				dbg << " ** English section titles:\n";
				for(const auto& sec : englishSections)
					dbg << sec.level << ": " << sec.title << "\n";
				dbg << " ** Translated section titles:\n";
				for(const auto& sec : translatedSections)
					dbg << sec.level << ": " << sec.title << "\n";
				dbg << "\n____________________________________________\n";
				sectionsOK = false;
				break;
			}
		}
		if(!sectionsOK) continue;

		TranslationDict dict;
		for(unsigned n = 0; n < englishSections.size(); ++n)
		{
			const auto& engSec = englishSections[n];
			if(engSec.level + engSec.levelAddition > 2) continue;

			QString key = engSec.body;
			QString value = translatedSections[n].body;
			auto titleForComment = engSec.title.contains(' ') ? '"' + engSec.title.toLower() + '"' : engSec.title.toLower();
			auto sectionTitle = engSec.title;
			bool insertDescriptionHeading = false;
			if(engSec.level == 1 && !key.isEmpty())
			{
				if(!finalEnglishSectionsDone)
					finalEnglishSections.emplace_back("Introduction", key);
				auto comment = QString("Sky culture introduction section in markdown format");
				dict.push_back({{std::move(comment)}, stripComments(key), std::move(value)});
				key = "";
				value = "";
				if(descrSectionExists) continue;

				titleForComment = "description";
				sectionTitle = "Description";
				insertDescriptionHeading = true;
			}

			for(const auto subN : engSec.subsections)
			{
				const auto& keySubSection = englishSections[subN];
				key += "\n\n";
				key += QString(keySubSection.level + keySubSection.levelAddition, QChar('#'));
				key += ' ';
				key += keySubSection.title;
				key += "\n\n";
				key += keySubSection.body;
				key += "\n\n";
				cleanupWhitespace(key);
				key.replace(QRegularExpression("^\n*|\\s*$"), "");

				const auto& valueSubSection = translatedSections[subN];
				value += "\n\n";
				value += QString(keySubSection.level + keySubSection.levelAddition, QChar('#'));
				value += ' ';
				value += valueSubSection.title;
				value += "\n\n";
				value += valueSubSection.body;
				value += "\n\n";
				cleanupWhitespace(value);
				value.replace(QRegularExpression("^\n*|\\s*$"), "");
			}
			if(!finalEnglishSectionsDone)
			{
				if((!sectionTitle.isEmpty() && engSec.level + engSec.levelAddition == 2) ||
				   insertDescriptionHeading)
					finalEnglishSections.emplace_back(sectionTitle, key);
			}
			if(!key.isEmpty())
			{
				auto comment = QString("Sky culture %1 section in markdown format").arg(titleForComment);
				dict.push_back({{std::move(comment)}, stripComments(key), std::move(value)});
			}
		}
		if(!finalEnglishSections.empty())
			finalEnglishSectionsDone = true;
		translations[locale] = std::move(dict);
	}

	// Reconstruct markdown from the altered sections
	if(finalEnglishSections.empty())
	{
		// A case where no translation exists
		markdown.clear();
		for(const auto& section : englishSections)
		{
			markdown += QString(section.level + section.levelAddition, QChar('#'));
			markdown += ' ';
			markdown += section.title.trimmed();
			markdown += "\n\n";
			markdown += section.body;
			markdown += "\n\n";
		}
	}
	else
	{
		markdown = "# " + englishSections[0].title + "\n\n";
		for(const auto& section : finalEnglishSections)
		{
			markdown += "## ";
			markdown += section.first;
			markdown += "\n\n";
			markdown += section.second;
			markdown += "\n\n";
		}
	}

	addMissingTextToMarkdown(markdown, inDir, author, credit, license);
	if(genTranslatedMD)
	{
		for(const auto& locale : locales)
			translatedMDs[locale] = translateDescription(markdown, locale);
	}

	loadTranslationsOfNames(poBaseDir, cultureId, englishName, consLoader, astLoader, namesLoader);
}

bool DescriptionOldLoader::dumpMarkdown(const QString& outDir) const
{
	const auto path = outDir+"/description.md";
	QFile file(path);
	if(!file.open(QFile::WriteOnly))
	{
		qCritical().noquote() << "Failed to open file" << path << "\n";
		return false;
	}

	if(markdown.isEmpty()) return true;

	if(file.write(markdown.toUtf8()) < 0 || !file.flush())
	{
		qCritical().noquote() << "Failed to write " << path << ": " << file.errorString() << "\n";
		return false;
	}

	for(const auto& img : imageHRefs)
	{
		const auto imgInPath = inputDir+"/"+img.inputPath;
		const auto imgInInfo = QFileInfo(imgInPath);
		if(!imgInInfo.exists())
		{
			qCritical() << "Failed to locate an image referenced in the description:" << img.inputPath;
			continue;
		}
		const auto imgOutPath = outDir + "/" + img.outputPath;
		const auto imgOutInfo = QFileInfo(imgOutPath);
		bool targetExistsAndDiffers = imgOutInfo.exists() && imgInInfo.size() != imgOutInfo.size();
		if(imgOutInfo.exists() && !targetExistsAndDiffers)
		{
			// Check that the contents are the same
			QFile in(imgInPath);
			if(!in.open(QFile::ReadOnly))
			{
				qCritical().noquote() << "Failed to open file" << imgInPath;
				continue;
			}
			QFile out(imgOutPath);
			if(!out.open(QFile::ReadOnly))
			{
				qCritical().noquote() << "Failed to open file" << imgOutPath;
				continue;
			}
			targetExistsAndDiffers = in.readAll() != out.readAll();
		}
		if(targetExistsAndDiffers)
		{
			qCritical() << "Image file names collide:" << img.inputPath << "and" << img.outputPath;
			continue;
		}

		if(imgOutInfo.exists())
		{
			// It exists, but is the same as the file we're trying to copy, so skip it
			continue;
		}

		const auto imgDir = QFileInfo(imgOutPath).absoluteDir().absolutePath();
		if(!QDir().mkpath(imgDir))
		{
			qCritical() << "Failed to create output directory for image file" << img.outputPath;
			continue;
		}

		if(!QFile(imgInPath).copy(imgOutPath))
		{
			qCritical() << "Failed to copy an image file referenced in the description:" << img.inputPath << "to" << img.outputPath;
			continue;
		}
	}

	if(!translatedMDs.isEmpty())
	{
		for(const auto& key : translatedMDs.keys())
		{
			const auto path = outDir+"/description."+key+".DO_NOT_COMMIT.md";
			QFile file(path);
			if(!file.open(QFile::WriteOnly))
			{
				qCritical().noquote() << "Failed to open file" << path << "\n";
				return false;
			}
			if(file.write(translatedMDs[key].toUtf8()) < 0 || !file.flush())
			{
				qCritical().noquote() << "Failed to write " << path << ": " << file.errorString() << "\n";
				return false;
			}
		}
	}

	return true;
}

bool DescriptionOldLoader::dump(const QString& outDir) const
{
	if(!dumpMarkdown(outDir)) return false;

	const auto poDir = outDir + "/po";
	if(!QDir().mkpath(poDir))
	{
		qCritical() << "Failed to create po directory\n";
		return false;
	}

	for(auto dictIt = translations.begin(); dictIt != translations.end(); ++dictIt)
	{
		const auto& locale = dictIt.key();
		const auto path = poDir + "/" + locale + ".po";

		const auto file = po_file_create();
		po_message_iterator_t iterator = po_message_iterator(file, nullptr);

		// I've found no API to *create* a header, so will try to emulate it with a message
		const auto headerIt = poHeaders.find(locale);
		if(headerIt == poHeaders.end())
		{
			qWarning().nospace() << "WARNING: No header for locale " << locale << " found. "
				"This could mean that either this locale is not supported by Stellarium, "
				"or its designation in the description file name is wrong.";
		}
		static const auto defaultHeaderTemplate = QLatin1String(
			"Project-Id-Version: PACKAGE VERSION\n"
			"MIME-Version: 1.0\n"
			"Content-Type: text/plain; charset=UTF-8\n"
			"Content-Transfer-Encoding: 8bit\n"
			"Language: %1\n");
		const QString header = headerIt == poHeaders.end() ?
			defaultHeaderTemplate.arg(locale) :
			headerIt.value();
		const auto headerMsg = po_message_create();
		po_message_set_msgid(headerMsg, "");
		po_message_set_msgstr(headerMsg, header.toStdString().c_str());
		po_message_insert(iterator, headerMsg);

		std::set<DictEntry> emittedEntries;
		for(const auto& entry : dictIt.value())
		{
			if(emittedEntries.find(entry) != emittedEntries.end()) continue;
			const auto msg = po_message_create();
			if(!entry.comment.empty())
				po_message_set_extracted_comments(msg, join(entry.comment).toStdString().c_str());
			po_message_set_msgid(msg, entry.english.toStdString().c_str());
			po_message_set_msgstr(msg, entry.translated.toStdString().c_str());
			po_message_insert(iterator, msg);
			emittedEntries.insert(entry);
		}
		po_message_iterator_free(iterator);
		po_xerror_handler handler = {gettextpo_xerror, gettextpo_xerror2};
		po_file_write(file, path.toStdString().c_str(), &handler);
		po_file_free(file);
	}
	return true;
}
