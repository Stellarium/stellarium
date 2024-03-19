#include <cmath>
#include <limits>
#include <QDir>
#include <QFile>
#include <QDebug>
#include <QRegularExpression>

#include "AsterismOldLoader.hpp"

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
private:
	//! International name (translated using gettext)
	QString nameI18;
	//! Name in english (second column in asterism_names.eng.fab)
	QString englishName;
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

	friend class AsterismOldLoader;
public:
	bool read(const QString& record);
};

std::ostream& operator<<(std::ostream& s, const Asterism::Star& star)
{
	if (star.HIP > 0)
		s << star.HIP;
	else
		s << "[" << star.RA << ", " << star.DE << "]";
	return s;
}

bool Asterism::read(const QString& record)
{
	abbreviation.clear();
	numberOfSegments = 0;
	typeOfAsterism = 1;
	flagAsterism = true;

	QString buf(record);
	QTextStream istr(&buf, QIODevice::ReadOnly);
	// We allow mixed-case abbreviations now that they can be displayed on screen. We then need toUpper() in comparisons.
	istr >> abbreviation >> typeOfAsterism >> numberOfSegments;
	if (istr.status()!=QTextStream::Ok)
		return false;

	asterism.resize(numberOfSegments*2);
	for (unsigned int i=0;i<numberOfSegments*2;++i)
	{
		switch (typeOfAsterism)
		{
			case 0: // Ray helpers
			case 1: // A big asterism with lines by HIP stars
			{
				unsigned int HP = 0;
				istr >> HP;
				if(HP == 0)
				{
					return false;
				}
				asterism[i] = Star{int(HP), NAN, NAN};
				break;
			}
			case 2: // A small asterism with lines by J2000.0 coordinates
			{
				double RA, DE;
				istr >> RA >> DE;
				asterism[i] = Star{-1, RA, DE};
				break;
			}
		}
	}

	return true;
}

Asterism* AsterismOldLoader::findFromAbbreviation(const QString& abbrev) const
{
	for (const auto asterism : asterisms)
		if (asterism->abbreviation == abbrev)
			return asterism;
	return nullptr;
}

void AsterismOldLoader::load(const QString& skyCultureDir)
{
	QString fic = skyCultureDir+"/asterism_lines.fab";
	if (fic.isEmpty())
	{
		hasAsterism = false;
		qWarning() << "No asterisms in " << skyCultureDir;
	}
	else
	{
		hasAsterism = true;
		loadLines(fic);
	}

	// load asterism names
	fic = skyCultureDir + "/asterism_names.eng.fab";
	if (!fic.isEmpty())
		loadNames(fic);
}

void AsterismOldLoader::loadLines(const QString &fileName)
{
	QFile in(fileName);
	if (!in.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qWarning() << "Can't open asterism data file" << QDir::toNativeSeparators(fileName);
		return;
	}

	int totalRecords=0;
	QString record;
	static const QRegularExpression commentRx("^(\\s*#.*|\\s*)$");
	while (!in.atEnd())
	{
		record = QString::fromUtf8(in.readLine());
		if (!commentRx.match(record).hasMatch())
			totalRecords++;
	}
	in.seek(0);

	// delete existing data, if any
	for (auto* asterism : asterisms)
		delete asterism;

	asterisms.clear();
	Asterism *aster = Q_NULLPTR;

	// read the file of line patterns, adding a record per non-comment line
	int currentLineNumber = 0;	// line in file
	int readOk = 0;			// count of records processed OK
	while (!in.atEnd())
	{
		record = QString::fromUtf8(in.readLine());
		currentLineNumber++;
		if (commentRx.match(record).hasMatch())
			continue;

		aster = new Asterism;
		if(aster->read(record))
		{
			asterisms.push_back(aster);
			++readOk;
		}
		else
		{
			qWarning() << "ERROR reading asterism lines record at line " << currentLineNumber;
			delete aster;
		}
	}
	in.close();
	qDebug() << "Loaded" << readOk << "/" << totalRecords << "asterism records successfully";
}

void AsterismOldLoader::loadNames(const QString& namesFile)
{
	// Asterism not loaded yet
	if (asterisms.empty()) return;

	// clear previous names
	for (auto* asterism : asterisms)
	{
		asterism->englishName.clear();
	}

	// Open file
	QFile commonNameFile(namesFile);
	if (!commonNameFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qDebug() << "Cannot open file" << QDir::toNativeSeparators(namesFile);
		return;
	}

	// Now parse the file
	// lines to ignore which start with a # or are empty
	static const QRegularExpression commentRx("^(\\s*#.*|\\s*)$");
	static const QRegularExpression recRx("^\\s*(\\S+)\\s+_[(]\"(.*)\"[)]\\s*([\\,\\d\\s]*)\\n");
	static const QRegularExpression ctxRx("(.*)\",\\s*\"(.*)");

	// keep track of how many records we processed.
	int totalRecords=0;
	int readOk=0;
	int lineNumber=0;
	while (!commonNameFile.atEnd())
	{
		QString record = QString::fromUtf8(commonNameFile.readLine());
		lineNumber++;

		// Skip comments
		if (commentRx.match(record).hasMatch())
			continue;

		totalRecords++;

		QRegularExpressionMatch recMatch=recRx.match(record);
		if (!recMatch.hasMatch())
		{
			qWarning() << "ERROR - cannot parse record at line" << lineNumber << "in asterism names file" << QDir::toNativeSeparators(namesFile) << ":" << record;
		}
		else
		{
			QString shortName = recMatch.captured(1);
			Asterism *aster = findFromAbbreviation(shortName);
			// If the asterism exists, set the English name
			if (aster != Q_NULLPTR)
			{
				QString ctxt = recMatch.captured(2);
				QRegularExpressionMatch ctxMatch=ctxRx.match(ctxt);
				if (ctxMatch.hasMatch())
				{
					aster->englishName = ctxMatch.captured(1);
					aster->context = ctxMatch.captured(2);
				}
				else
				{
					aster->englishName = ctxt;
					aster->context = "";
				}
				readOk++;
			}
			else
			{
				qWarning() << "WARNING - asterism abbreviation" << shortName << "not found when loading asterism names";
			}
		}
	}
	commonNameFile.close();
	qDebug() << "Loaded" << readOk << "/" << totalRecords << "asterism names";
}

bool AsterismOldLoader::dumpJSON(std::ostream& s) const
{
	if (!hasAsterism) return false;

	s << "  \"asterisms\": [\n";
	for (const Asterism*const ast : asterisms)
	{
		s << "    {\n";
		s << "      \"id\": \"" << ast->abbreviation.toStdString() << "\",\n";
		if (!ast->englishName.isEmpty())
			s << "      \"common_name\": {\"english\": \"" << ast->englishName.toStdString() << "\"},\n";
		s << "      \"is_ray_helper\": " << (ast->typeOfAsterism == 0 ? "true" : "false") << ",\n";

		s.precision(std::numeric_limits<double>::digits10);
		s << "      \"lines\": [";
		auto& points = ast->asterism;
		for (unsigned n = 1; n < points.size(); n += 2)
		{
			s << (n > 1 ? ", [" : "[") << points[n - 1] << ", " << points[n];
			// Merge connected segments into polylines
			while (n + 2 < points.size() && points[n + 1] == points[n])
			{
				s << ", " << points[n + 2];
				n += 2;
			}
			s << "]";
		}
		s << "]\n";
		if (ast == asterisms.back())
			s << "    }\n";
		else
			s << "    },\n";
	}
	s << "  ],\n";

	return true;
}
