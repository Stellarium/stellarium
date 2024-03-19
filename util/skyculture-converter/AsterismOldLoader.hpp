#pragma once

#include <vector>
#include <QString>

class Asterism;
class AsterismOldLoader
{
	bool hasAsterism = false;
	std::vector<Asterism*> asterisms;

	Asterism* findFromAbbreviation(const QString& abbrev) const;
	void loadLines(const QString& fileName);
	void loadNames(const QString& namesFile);
public:
	void load(const QString& skyCultureDir);
	bool dumpJSON(std::ostream& s) const;
};
