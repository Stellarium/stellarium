#include "DescriptionOldLoader.hpp"

#include <QDir>
#include <QFile>
#include <QDebug>
#include <QRegularExpression>

namespace
{

#if (QT_VERSION>=QT_VERSION_CHECK(5, 14, 0))
constexpr auto SkipEmptyParts = Qt::SkipEmptyParts;
#else
constexpr auto SkipEmptyParts = QString::SkipEmptyParts;
#endif

void htmlListsToMarkdown(QString& string)
{
	// This will only handle lists whose entries don't contain HTML tags, and the
	// lists don't contain anything except <li> entries (in particular, no comments).
	static const QRegularExpression listPattern(R"reg(<ul\s*>\s*(?:<li\s*>[^<]+</li\s*>\s*)+</ul\s*>)reg");
	static const QRegularExpression outerTagPattern(R"reg(</?ul\s*>)reg");
	static const QRegularExpression entryPattern(R"reg(<li\s*>([^<]+)</li\s*>)reg");
	for(auto matches = listPattern.globalMatch(string); matches.hasNext(); )
	{
		const auto& match = matches.next();
		auto list = match.captured(0);
		list.replace(outerTagPattern, "\n");
		list.replace(entryPattern, "\n * \\1\n");
		string.replace(match.captured(0), list);
	}
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
		}
		else if(startCap.isEmpty() && !endCap.isEmpty() && foundStart)
		{
			foundStart = false;
			Q_ASSERT(startPos >= 0);
			Q_ASSERT(tagStartPos >= 0);
			const auto endPos = match.capturedStart(2);
			const auto tagEndPos = match.capturedEnd(2);
			tables += string.mid(startPos, endPos - startPos);
			tablesPositions.emplace_back(tagStartPos, tagEndPos);
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
		if(table.contains(QRegularExpression("\\s(?:col|row)span=|<th\\s*>")))
		{
			qWarning() << "Row/column spans and <th> tags are not supported, leaving the table in HTML form";
			continue;
		}
		if(!table.contains(QRegularExpression("^\\s*<tr\\s*>")))
		{
			qWarning() << "Unexpected table contents (expected it to start with <tr>), keeping the table in HTML form";
			continue;
		}
		if(!table.contains(QRegularExpression("</tr\\s*>\\s*$")))
		{
			qWarning() << "Unexpected table contents (expected it to end with </tr>), keeping the table in HTML form";
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
			if(!row.contains(QRegularExpression("^\\s*<td\\s*>")))
			{
				qWarning() << "Unexpected row contents (expected it to start with <td>), keeping the table in HTML form. Row:" << row;
				goto nextTable;
			}
			if(!row.contains(QRegularExpression("</td\\s*>\\s*$")))
			{
				qWarning() << "Unexpected row contents (expected it to end with </td>), keeping the table in HTML form. Row:" << row;
				goto nextTable;
			}
			auto cols = row.split(QRegularExpression("\\s*</td\\s*>\\s*"), SkipEmptyParts);
			// The closing column tags have been removed by QString::split, now remove the opening tags
			static const QRegularExpression tdOpenTag("^\\s*<td\\s*>\\s*");
			for(auto& col : cols) col.replace(tdOpenTag, "");

			// Finally, emit the rows
			const bool firstRow = markdownTable.isEmpty();
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
					markdownTable += QString(std::max(3, col.size()), QChar('-'));
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
			reference.append(QString(" %1. %2\n").arg(ref[0], ref[1]));
			readOk++;
		}
		else
		{
			if (ref.at(2).isEmpty())
				reference.append(QString(" %1. %2\n").arg(ref[0], ref[1]));
			else
				reference.append(QString(" %1. [%2](%3)\n").arg(ref[0], ref[1], ref[2]));
			readOk++;
		}
	}
	if(readOk != totalRecords)
		qDebug() << "Loaded" << readOk << "/" << totalRecords << "references";

	return reference;
}

}

void DescriptionOldLoader::load(const QString& inDir, const QString& author, const QString& credit, const QString& license)
{
	const auto path = inDir+"/description.en.utf8";
	QFile file(path);
	if(!file.open(QFile::ReadOnly))
	{
		qCritical().noquote() << "Failed to open file" << path << "\n";
		return;
	}

	const auto html = file.readAll();
	markdown = html;

	// Replace <notr> and </notr> tags with placeholders that don't
	// look like tags, so as not to confuse the replacements below.
	const QString notrOpenPlaceholder = "[22c35d6a-5ec3-4405-aeff-e79998dc95f7]";
	const QString notrClosePlaceholder = "[2543be41-c785-4283-a4cf-ce5471d2c422]";
	markdown.replace(QRegularExpression("<notr\\s*>"), notrOpenPlaceholder);
	markdown.replace(QRegularExpression("</notr\\s*>"), notrClosePlaceholder);

	// Replace simple HTML headings with corresponding Markdown ones
	for(int n = 1; n <= 6; ++n)
		markdown.replace(QRegularExpression(QString("<h%1>([^<]+)</h%1>").arg(n)), QString(n, QChar('#'))+" \\1");

	// Replace HTML line breaks with the Markdown ones
	markdown.replace(QRegularExpression("<br\\s*/?>"), "\n\n");

	// Replace simple HTML italics with the Markdown ones
	markdown.replace(QRegularExpression("<i>\\s*([^<]+)\\s*</i>"), "*\\1*");
	markdown.replace(QRegularExpression("<em>\\s*([^<]+)\\s*</em>"), "*\\1*");

	// Replace simple HTML images with the Markdown ones
	markdown.replace(QRegularExpression(R"reg(<img(?:\s+alt="([^"]+)")?\s+src="([^"]+)"(?:\s+alt="([^"]+)")?/>)reg"),
	                 R"rep(![\1\3](\2))rep");

	// Replace simple HTML hyperlinks with the Markdown ones
	markdown.replace(QRegularExpression("<a\\s+href=\"([^\"]+)\"\\s*>([^<]+)</a\\s*>"), "[\\2](\\1)");

	// Replace simple HTML paragraphs with the Markdown ones
	markdown.replace(QRegularExpression("<p>([^<]+)</p>"), "\n\\1\n");

	// Add missing "Introduction" heading if we have a headingless intro text
	markdown.replace(QRegularExpression("^(\\s*# [^\n]+\n+)(\\s*[^#])"), "\\1## Introduction\n\n\\2");

	htmlTablesToMarkdown(markdown);

	// Add some sections the info for which is contained in info.ini in the old format
	if(markdown.contains(QRegularExpression("\n##\\s+(?:References|External\\s+links)\\s*\n")))
		markdown.replace(QRegularExpression("(\n##[ \t]+)External[ \t]+links([ \t]*\n)"), "\\1References\\2");
	const auto referencesFromFile = readReferencesFile(inDir);

	if(markdown.contains(QRegularExpression("\n##\\s+Authors?\\s*\n")))
	{
		qWarning() << "Authors section already exists, not adding the authors from info.ini";

		// But do add references before this section
		if(!referencesFromFile.isEmpty())
			markdown.replace(QRegularExpression("(\n##\\s+Authors?\\s*\n)"), referencesFromFile + "\n\\1");
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

	htmlListsToMarkdown(markdown);

	// Clean too long chains of newlines
	markdown.replace(QRegularExpression("\n\n\n+"), "\n\n");

	// Make lists a bit denser
	const QRegularExpression listSpaceListPattern("(\n \\*[^\n]+)\n+(\n \\*)");
	//  1. Remove space between odd and even entries
	markdown.replace(listSpaceListPattern, "\\1\\2");
	//  2. Remove space between even and odd entries (same replacement rule)
	markdown.replace(listSpaceListPattern, "\\1\\2");

	// Restore <notr> and </notr> tags
	markdown.replace(notrOpenPlaceholder,  "<notr>");
	markdown.replace(notrClosePlaceholder, "</notr>");
}

bool DescriptionOldLoader::dump(const QString& outDir) const
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

	return true;
}
