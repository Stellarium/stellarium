#pragma once

#include <tuple>
#include <vector>
#include <QHash>
#include <QString>

class DescriptionOldLoader
{
	QString markdown;
	QHash<QString/*locale*/, QString/*markdown*/> translatedMDs;
	QString inputDir;
	std::vector<QString> imageHRefs;
	struct DictEntry
	{
		QString comment;
		QString extractedComment;
		QString english;
		QString translated;
		bool operator<(const DictEntry& other) const
		{
			return std::tie(comment,english,translated) < std::tie(other.comment,other.english,other.translated);
		}
	};
	using TranslationDict = std::vector<DictEntry>;
	QHash<QString/*locale*/, TranslationDict> translations;
	QHash<QString/*locale*/, QString/*header*/> poHeaders;
	bool dumpMarkdown(const QString& outDir) const;
	void locateAllInlineImages(const QString& html);
	void loadTranslationsOfNames(const QString& poBaseDir, const QString& cultureId, const QString& englishName);
	QString translateSection(const QString& markdown, const qsizetype bodyStartPos, const qsizetype bodyEndPos, const QString& locale, const QString& sectionName) const;
	QString translateDescription(const QString& markdown, const QString& locale) const;
public:
	void load(const QString& inDir, const QString& poBaseDir, const QString& cultureId, const QString& englishName,
	          const QString& author, const QString& credit, const QString& license,
	          bool fullerConversionToMarkdown, bool footnotesToRefs, bool convertOrderedLists, bool genTranslatedMD);
	bool dump(const QString& outDir) const;
};
