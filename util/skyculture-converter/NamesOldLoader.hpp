#pragma once

#include <iostream>
#include <QString>
#include <QMultiHash>

class NamesOldLoader
{
	QMultiHash<int, QString> starNames;
	QMultiHash<QString/*dsoId*/,QString/*name*/> dsoNames;
	QMultiHash<QString/*dsoId*/,QString/*name*/> planetNames;
	QMultiHash<QString/*dsoId*/,QString/*name*/> planetNamesMeaning;

	void loadStarNames(const QString& skyCultureDir);
	void loadDSONames(const QString& skyCultureDir);
	void loadPlanetNames(const QString& skyCultureDir);
public:
	void load(const QString& skyCultureDir);
	bool dumpJSON(std::ostream& s) const;
};
