#pragma once

#include <vector>
#include <iostream>
#include <QSize>
#include <QString>

class ConstellationOldLoader
{
public:
	struct Constellation
	{
		QString abbreviation;
		QString englishName;
		QString nativeName;
		std::vector<int> references;
		QString translatorsComments;
		QString artTexture;
		QSize textureSize;
		std::vector<int> points;
		struct Point
		{
			int hip;
			int x, y;
		} artP1, artP2, artP3;
		int beginSeason = 1;
		int endSeason = 12;
		bool seasonalRuleEnabled = false;

		bool read(QString const& record);
	};
private:
	QString skyCultureName;
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
	void loadSeasonalRules(const QString& rulesFile);
	bool dumpBoundariesJSON(std::ostream& s) const;
	bool dumpConstellationsJSON(std::ostream& s) const;
public:
	void load(const QString &skyCultureDir, const QString& outDir);
	const Constellation* find(QString const& englishName) const;
	bool dumpJSON(std::ostream& s) const;
	bool hasBoundaries() const { return !boundaries.empty(); }
	void setBoundariesType(std::string const& type) { boundariesType = type; }
};
