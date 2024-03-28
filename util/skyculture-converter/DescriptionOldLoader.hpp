#pragma once

#include <QHash>
#include <QString>

class DescriptionOldLoader
{
	QString markdown;
	using TranslationDict = QHash<QString/*english*/, QString/*translated*/>;
	QHash<QString/*locale*/, TranslationDict> translations;
	bool dumpMarkdown(const QString& outDir) const;
public:
	void load(const QString& inDir, const QString& author, const QString& credit, const QString& license);
	bool dump(const QString& outDir) const;
};
