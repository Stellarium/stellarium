#pragma once

#include <cmath>
#include <vector>
#include <QString>

class AsterismOldLoader
{
public:
	class Asterism
	{
	public:
		struct Star
		{
			int HIP = -1;
			double RA = NAN, DE = NAN;
			bool operator==(const Star& rhs) const
			{
				if (HIP > 0) return HIP == rhs.HIP;
				return RA == rhs.RA && DE == rhs.DE;
			}
		};
		QString getTranslatorsComments() const { return translatorsComments; }
	private:
		//! International name (translated using gettext)
		QString nameI18;
		//! Name in english (second column in asterism_names.eng.fab)
		QString englishName;
		//! Extracted comments
		QString translatorsComments;
		//! Abbreviation
		//! A skyculture designer must invent it. (usually 2-5 letters)
		//! This MUST be filled and be unique within a sky culture.
		QString abbreviation;
		//! Context for name
		QString context;
		//! Number of segments in the lines
		unsigned int numberOfSegments;
		//! Type of asterism
		int typeOfAsterism;
		bool flagAsterism;

		std::vector<Star> asterism;
		std::vector<int> references;

		friend class AsterismOldLoader;
	public:
		bool read(const QString& record);
	};

	void load(const QString& skyCultureDir, const QString& cultureId);
	const Asterism* find(QString const& englishName) const;
	bool dumpJSON(std::ostream& s) const;
private:
	QString cultureId;
	bool hasAsterism = false;
	std::vector<Asterism*> asterisms;

	Asterism* findFromAbbreviation(const QString& abbrev) const;
	void loadLines(const QString& fileName);
	void loadNames(const QString& namesFile);
};
