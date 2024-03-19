#pragma once

#include <QString>

class DescriptionOldLoader
{
	QString markdown;
public:
	void load(const QString& inDir, const QString& author, const QString& credit, const QString& license);
	bool dump(const QString& outDir) const;
};
