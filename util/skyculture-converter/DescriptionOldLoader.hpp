#pragma once

#include <set>
#include <tuple>
#include <vector>
#include <QHash>
#include <QString>

class ConstellationOldLoader;
class AsterismOldLoader;
class NamesOldLoader;
class DescriptionOldLoader
{
	QString markdown;
	QHash<QString/*locale*/, QString/*markdown*/> translatedMDs;
	QString inputDir;
	struct ImageHRef
	{
		ImageHRef(QString const& inputPath, QString const& outputPath)
			: inputPath(inputPath), outputPath(outputPath)
		{}
		QString inputPath;
		QString outputPath;
	};
	std::vector<ImageHRef> imageHRefs;
	struct DictEntry
	{
		std::set<QString> comment;
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
	std::set<DictEntry> allMarkdownSections;
	bool dumpMarkdown(const QString& outDir) const;
	void locateAndRelocateAllInlineImages(QString& html, bool saveToRefs);
	void loadTranslationsOfNames(const QString& poBaseDir, const QString& cultureId, const QString& englishName,
	                             const ConstellationOldLoader& consLoader, const AsterismOldLoader& astLoader, const NamesOldLoader& namesLoader);
	QString translateSection(const QString& markdown, const qsizetype bodyStartPos, const qsizetype bodyEndPos, const QString& locale, const QString& sectionName);
	QString translateDescription(const QString& markdown, const QString& locale);
public:
	void load(const QString& inDir, const QString& poBaseDir, const QString& cultureId, const QString& englishName,
	          const QString& author, const QString& credit, const QString& license,
	          const ConstellationOldLoader& consLoader, const AsterismOldLoader& astLoader, const NamesOldLoader& namesLoader,
	          bool fullerConversionToMarkdown, bool footnotesToRefs, bool convertOrderedLists, bool genTranslatedMD);
	bool dump(const QString& outDir) const;
};
