#pragma once

#include <vector>
#include <iostream>
#include <QSize>
#include <QString>

class ConstellationOldLoader
{
	QString skyCultureName;
	struct Constellation
	{
		QString abbreviation;
		QString englishName;
		QString nativeName;
		QString artTexture;
		QSize textureSize;
		std::vector<int> points;
		struct Point
		{
			int hip;
			int x, y;
		} artP1, artP2, artP3;

		bool read(QString const& record);
	};
	std::vector<Constellation> constellations;
	struct RaDec
	{
		double ra, dec;
	};
	struct BoundaryLine
	{
		std::vector<RaDec> points;
		QString cons1, cons2;
	};
	std::vector<BoundaryLine> boundaries;
	std::string boundariesType;

	Constellation* findFromAbbreviation(const QString& abbrev);
	void loadLinesAndArt(const QString &skyCultureDir, const QString& outDir);
	void loadBoundaries(const QString& skyCultureDir);
	void loadNames(const QString &skyCultureDir);
	bool dumpBoundariesJSON(std::ostream& s) const;
	bool dumpConstellationsJSON(std::ostream& s) const;
public:
	void load(const QString &skyCultureDir, const QString& outDir);
	bool dumpJSON(std::ostream& s) const;
	bool hasBoundaries() const { return !boundaries.empty(); }
	void setBoundariesType(std::string const& type) { boundariesType = type; }
};
