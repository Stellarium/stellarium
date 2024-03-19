#include "NamesOldLoader.hpp"

#include <string>
#include <QDir>
#include <QFile>
#include <QDebug>
#include <QRegularExpression>

void NamesOldLoader::loadStarNames(const QString& skyCultureDir)
{
	const auto nameFile = skyCultureDir + "/star_names.fab";
	QFile cnFile(nameFile);
	if (!cnFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qWarning().noquote() << "WARNING - could not open" << QDir::toNativeSeparators(nameFile);
		return;
	}

	int readOk=0;
	int totalRecords=0;
	int lineNumber=0;
	QString record;
	// Allow empty and comment lines where first char (after optional blanks) is #
	static const QRegularExpression commentRx("^(\\s*#.*|\\s*)$");
	// record structure is delimited with a | character.  We will
	// use a QRegularExpression to extract the fields. with white-space padding permitted
	// (i.e. it will be stripped automatically) Example record strings:
	// "   677|_("Alpheratz")"
	// "113368|_("Fomalhaut")"
	// Note: Stellarium doesn't support sky cultures made prior to version 0.10.6 now!
	static const QRegularExpression recordRx("^\\s*(\\d+)\\s*\\|[_]*[(]\"(.*)\"[)]\\s*([\\,\\d\\s]*)");

	while(!cnFile.atEnd())
	{
		record = QString::fromUtf8(cnFile.readLine()).trimmed();
		lineNumber++;
		if (commentRx.match(record).hasMatch())
			continue;

		totalRecords++;
		QRegularExpressionMatch recMatch=recordRx.match(record);
		if (!recMatch.hasMatch())
		{
			qWarning().noquote() << "WARNING - parse error at line" << lineNumber << "in" << QDir::toNativeSeparators(nameFile)
				   << " - record does not match record pattern";
			qWarning().noquote() << "Problematic record:" << record;
			continue;
		}
		else
		{
			// The record is the right format.  Extract the fields
			bool ok;
			const int hip = recMatch.captured(1).toInt(&ok);
			if (!ok)
			{
				qWarning().noquote() << "WARNING - parse error at line" << lineNumber << "in" << QDir::toNativeSeparators(nameFile)
					   << " - failed to convert " << recMatch.captured(1) << "to a number";
				continue;
			}
			QString englishCommonName = recMatch.captured(2).trimmed();
			if (englishCommonName.isEmpty())
			{
				qWarning().noquote() << "WARNING - parse error at line" << lineNumber << "in" << QDir::toNativeSeparators(nameFile)
					   << " - empty name field";
				continue;
			}

			starNames.insert(hip, englishCommonName);

			readOk++;
		}
	}
	cnFile.close();

	if(readOk != totalRecords)
		qDebug().noquote() << "Loaded" << readOk << "/" << totalRecords << "common star names";
}

void NamesOldLoader::loadDSONames(const QString& skyCultureDir)
{
	const auto namesFile = skyCultureDir + "/dso_names.fab";
	if (namesFile.isEmpty())
	{
		qWarning() << "Failed to open file" << QDir::toNativeSeparators(namesFile);
		return;
	}

	// Open file
	QFile dsoNamesFile(namesFile);
	if (!dsoNamesFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qWarning() << "Failed to open file" << QDir::toNativeSeparators(namesFile);
		return;
	}

	// Now parse the file
	// lines to ignore which start with a # or are empty
	static const QRegularExpression commentRx("^(\\s*#.*|\\s*)$");

	// lines which look like records - we use the RE to extract the fields
	// which will be available in recMatch.capturedTexts()
	static const QRegularExpression recRx("^\\s*([\\w\\s\\-\\+\\.]+)\\s*\\|[_]*[(]\"(.*)\"[)]\\s*([\\,\\d\\s]*)");

	QString record, dsoId, nativeName;
	int totalRecords=0;
	int readOk=0;
	int lineNumber=0;
	while (!dsoNamesFile.atEnd())
	{
		record = QString::fromUtf8(dsoNamesFile.readLine()).trimmed();
		lineNumber++;

		// Skip comments
		if (commentRx.match(record).hasMatch())
			continue;

		totalRecords++;

		QRegularExpressionMatch recMatch=recRx.match(record);
		if (!recMatch.hasMatch())
		{
			qWarning().noquote() << "ERROR - cannot parse record at line" << lineNumber << "in native deep-sky object names file" << QDir::toNativeSeparators(namesFile);
		}
		else
		{
			dsoId = recMatch.captured(1).trimmed();
			nativeName = recMatch.captured(2).trimmed(); // Use translatable text
			dsoNames.insert(dsoId, nativeName);

			readOk++;
		}
	}
	dsoNamesFile.close();
	if(readOk != totalRecords)
		qDebug().noquote() << "Loaded" << readOk << "/" << totalRecords << "common names of deep-sky objects";
}

void NamesOldLoader::loadPlanetNames(const QString& skyCultureDir)
{
	const auto namesFile = skyCultureDir + "/planet_names.fab";
	// Open file
	QFile planetNamesFile(namesFile);
	if (!planetNamesFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qWarning() << "Failed to open file" << QDir::toNativeSeparators(namesFile);
		return;
	}

	// Now parse the file
	// lines to ignore which start with a # or are empty
	static const QRegularExpression commentRx("^(\\s*#.*|\\s*)$");

	// lines which look like records - we use the RE to extract the fields
	// which will be available in recRx.capturedTexts()
	static const QRegularExpression recRx("^\\s*(\\w+)\\s+\"(.+)\"\\s+_[(]\"(.+)\"[)]\\n");

	// keep track of how many records we processed.
	int totalRecords=0;
	int readOk=0;
	int lineNumber=0;
	while (!planetNamesFile.atEnd())
	{
		const auto record = QString::fromUtf8(planetNamesFile.readLine());
		lineNumber++;

		// Skip comments
		if (commentRx.match(record).hasMatch())
			continue;

		totalRecords++;

		QRegularExpressionMatch match=recRx.match(record);
		if (!match.hasMatch())
		{
			qWarning() << "ERROR - cannot parse record at line" << lineNumber << "in planet names file" << QDir::toNativeSeparators(namesFile);
		}
		else
		{
			const auto planetId          = match.captured(1).trimmed();
			const auto nativeName        = match.captured(2).trimmed();
			const auto nativeNameMeaning = match.captured(3).trimmed();
			planetNames.insert(planetId, nativeName);
			planetNamesMeaning.insert(planetId, nativeNameMeaning);
			readOk++;
		}
	}
}

void NamesOldLoader::load(const QString& skyCultureDir)
{
	loadStarNames(skyCultureDir);
	loadDSONames(skyCultureDir);
	loadPlanetNames(skyCultureDir);
}

bool NamesOldLoader::dumpJSON(std::ostream& s) const
{
	if (starNames.isEmpty() && dsoNames.isEmpty() && planetNames.isEmpty())
		return false;
	s << "  \"common_names\": {\n";
	const auto starKeys = starNames.uniqueKeys();
	for(int k = 0; k < starKeys.size(); ++k)
	{
		const auto& key = starKeys[k];
		s << "    \"HIP " + std::to_string(key) + "\": [";
		const auto values = starNames.values(key);
		for(int v = 0; v < values.size(); ++v)
		{
			s << ("{\"english\": \"" + values[v] + "\"}").toStdString();
			if(v+1 != values.size()) s << ", ";
		}
		if(k+1 != starKeys.size() || !dsoNames.isEmpty() || !planetNames.isEmpty())
			s << "],\n";
		else
			s << "]\n";
	}

	const auto dsoKeys = dsoNames.uniqueKeys();
	for(int k = 0; k < dsoKeys.size(); ++k)
	{
		const auto& key = dsoKeys[k];
		s << ("    \"" + key + "\": [").toStdString();
		const auto values = dsoNames.values(key);
		for(int v = 0; v < values.size(); ++v)
		{
			s << ("{\"english\": \"" + values[v] + "\"}").toStdString();
			if(v+1 != values.size()) s << ", ";
		}
		if(k+1 != dsoKeys.size() || !planetNames.isEmpty())
			s << "],\n";
		else
			s << "]\n";
	}

	const auto planetKeys = planetNames.uniqueKeys();
	for(int k = 0; k < planetKeys.size(); ++k)
	{
		const auto& key = planetKeys[k];
		s << ("    \"NAME " + key + "\": [").toStdString();
		const auto values = planetNames.values(key);
		for(int v = 0; v < values.size(); ++v)
		{
			s << ("{\"english\": \"" + values[v] + "\"}").toStdString();
			if(v+1 != values.size()) s << ", ";
		}
		if(k+1 != planetKeys.size())
			s << "],\n";
		else
			s << "]\n";
	}

	s << "  },\n";
	return true;
}
