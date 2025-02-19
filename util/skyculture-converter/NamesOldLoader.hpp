#pragma once

#include <vector>
#include <iostream>
#include <QMap>
#include <QString>

class NamesOldLoader
{
public:
	struct StarName
	{
		int HIP;
		QString englishName;
		QString nativeName;
		QString translatorsComments;
		std::vector<int> references;
	};
	struct DSOName
	{
		QString id;
		QString englishName;
		QString nativeName;
		QString translatorsComments;
		std::vector<int> references;
	};
	struct PlanetName
	{
		QString id;
		QString english;
		QString native;
		QString translatorsComments;
	};
	void load(const QString& skyCultureDir, bool convertUntranslatableNamesToNative);
	const StarName* findStar(QString const& englishName) const;
	const DSOName* findDSO(QString const& englishName) const;
	const PlanetName* findPlanet(QString const& englishName) const;
	bool dumpJSON(std::ostream& s) const;

private:
	void loadStarNames(const QString& skyCultureDir, bool convertUntranslatableNamesToNative);
	void loadDSONames(const QString& skyCultureDir, bool convertUntranslatableNamesToNative);
	void loadPlanetNames(const QString& skyCultureDir);
	QMap<int/*HIP*/, std::vector<StarName>> starNames;
	QMap<QString/*dsoId*/,std::vector<DSOName>> dsoNames;
	QMap<QString/*planetId*/,std::vector<PlanetName>> planetNames;
};
