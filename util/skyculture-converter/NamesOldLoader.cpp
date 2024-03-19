#include "NamesOldLoader.hpp"

#include <string>
#include <QDir>
#include <QFile>
#include <QDebug>
#include <QRegularExpression>
#include "Utils.hpp"

template<typename Map>
void coalesceEnglishAndNativeNamesIntoSingleEntries(Map& data)
{
	for(auto it = data.begin(); it != data.end(); ++it)
	{
		auto& star = it.value();
		if(star.size() != 2) continue;
		if(star[0].englishName.isEmpty() && !star[0].nativeName.isEmpty() &&
		   !star[1].englishName.isEmpty() && star[1].nativeName.isEmpty())
		{
			star[0].englishName = std::move(star[1].englishName);
			star.pop_back();
		}
		else if(star[1].englishName.isEmpty() && !star[1].nativeName.isEmpty() &&
		        !star[0].englishName.isEmpty() && star[0].nativeName.isEmpty())
		{
			star[0].nativeName = std::move(star[1].nativeName);
			star.pop_back();
		}
	}
}

void NamesOldLoader::loadStarNames(const QString& skyCultureDir, const bool convertUntranslatableNamesToNative)
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
	static const QRegularExpression recordRx("^\\s*(\\d+)\\s*\\|(_*)[(]\"(.*)\"[)]\\s*([\\,\\d\\s]*)");

	QString translatorsComments;
	while(!cnFile.atEnd())
	{
		record = QString::fromUtf8(cnFile.readLine()).trimmed();
		lineNumber++;
		const auto commentMatch = commentRx.match(record);
		if (commentMatch.hasMatch())
		{
			const auto comment = commentMatch.captured(1).trimmed().replace(QRegularExpression("^#\\s*"), "");
			if(comment.startsWith(translatorsCommentPrefix))
				translatorsComments += comment.mid(translatorsCommentPrefix.size()).trimmed() + "\n";
			else if(!comment.isEmpty())
				translatorsComments = ""; // apparently previous translators' comment was not referring to the next entry
			continue;
		}

		totalRecords++;
		QRegularExpressionMatch recMatch=recordRx.match(record);
		if (!recMatch.hasMatch())
		{
			qWarning().noquote() << "WARNING - parse error at line" << lineNumber << "in" << QDir::toNativeSeparators(nameFile)
				   << " - record does not match record pattern";
			qWarning().noquote() << "Problematic record:" << record;
			translatorsComments = "";
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
				translatorsComments = "";
				continue;
			}
			QString name = recMatch.captured(3).trimmed();
			if (name.isEmpty())
			{
				qWarning().noquote() << "WARNING - parse error at line" << lineNumber << "in" << QDir::toNativeSeparators(nameFile)
					   << " - empty name field";
				translatorsComments = "";
				continue;
			}
			auto&& refs = parseReferences(recMatch.captured(4).trimmed());
			QString englishName, nativeName;
			if(recMatch.captured(2).isEmpty() && convertUntranslatableNamesToNative)
				nativeName = name;
			else
				englishName = name;

			starNames[hip].push_back({hip,englishName,nativeName,translatorsComments,std::move(refs)});
			translatorsComments = "";

			readOk++;
		}
	}
	cnFile.close();

	if(readOk != totalRecords)
		qDebug().noquote() << "Loaded" << readOk << "/" << totalRecords << "common star names";

	if(convertUntranslatableNamesToNative)
		coalesceEnglishAndNativeNamesIntoSingleEntries(starNames);
}

auto NamesOldLoader::findStar(QString const& englishName) const -> StarName const*
{
	for(auto it = starNames.cbegin(); it != starNames.end(); ++it)
		for(const auto& star : it.value())
			if(star.englishName == englishName)
				return &star;
	return nullptr;
}

void NamesOldLoader::loadDSONames(const QString& skyCultureDir, const bool convertUntranslatableNamesToNative)
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
	static const QRegularExpression recRx("^\\s*([\\w\\s\\-\\+\\.]+)\\s*\\|(_*)[(]\"(.*)\"[)]\\s*([\\,\\d\\s]*)");

	QString record, dsoId;
	int totalRecords=0;
	int readOk=0;
	int lineNumber=0;
	QString translatorsComments;
	while (!dsoNamesFile.atEnd())
	{
		record = QString::fromUtf8(dsoNamesFile.readLine()).trimmed();
		lineNumber++;

		const auto commentMatch = commentRx.match(record);
		if (commentMatch.hasMatch())
		{
			const auto comment = commentMatch.captured(1).trimmed().replace(QRegularExpression("^#\\s*"), "");
			if(comment.startsWith(translatorsCommentPrefix))
				translatorsComments += comment.mid(translatorsCommentPrefix.size()).trimmed() + "\n";
			else if(!comment.isEmpty())
				translatorsComments = ""; // apparently previous translators' comment was not referring to the next entry
			continue;
		}

		totalRecords++;

		QRegularExpressionMatch recMatch=recRx.match(record);
		if (!recMatch.hasMatch())
		{
			qWarning().noquote() << "ERROR - cannot parse record at line" << lineNumber << "in native deep-sky object names file" << QDir::toNativeSeparators(namesFile);
		}
		else
		{
			dsoId = recMatch.captured(1).trimmed();

			const auto name = recMatch.captured(3).trimmed(); // Use translatable text
			QString englishName, nativeName;
			if(recMatch.captured(2).isEmpty() && convertUntranslatableNamesToNative)
				nativeName = name;
			else
				englishName = name;

			auto&& refs = parseReferences(recMatch.captured(4).trimmed());
			dsoNames[dsoId].push_back({dsoId,englishName,nativeName,translatorsComments,std::move(refs)});

			readOk++;
		}
		translatorsComments = "";
	}
	dsoNamesFile.close();
	if(readOk != totalRecords)
		qDebug().noquote() << "Loaded" << readOk << "/" << totalRecords << "common names of deep-sky objects";

	if(convertUntranslatableNamesToNative)
		coalesceEnglishAndNativeNamesIntoSingleEntries(dsoNames);
}

auto NamesOldLoader::findDSO(QString const& englishName) const -> DSOName const*
{
	for(auto it = dsoNames.cbegin(); it != dsoNames.end(); ++it)
		for(const auto& dso : it.value())
			if(dso.englishName == englishName)
				return &dso;
	return nullptr;
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
	QString translatorsComments;
	while (!planetNamesFile.atEnd())
	{
		const auto record = QString::fromUtf8(planetNamesFile.readLine());
		lineNumber++;

		const auto commentMatch = commentRx.match(record);
		if (commentMatch.hasMatch())
		{
			const auto comment = commentMatch.captured(1).trimmed().replace(QRegularExpression("^#\\s*"), "");
			if(comment.startsWith(translatorsCommentPrefix))
				translatorsComments += comment.mid(translatorsCommentPrefix.size()).trimmed() + "\n";
			else if(!comment.isEmpty())
				translatorsComments = ""; // apparently previous translators' comment was not referring to the next entry
			continue;
		}


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
			planetNames[planetId].push_back({planetId,nativeNameMeaning,nativeName,translatorsComments});
			readOk++;
		}
		translatorsComments = "";
	}
}

auto NamesOldLoader::findPlanet(QString const& englishName) const -> PlanetName const*
{
	for(auto it = planetNames.cbegin(); it != planetNames.end(); ++it)
		for(const auto& planet : it.value())
			if(planet.english == englishName)
				return &planet;
	return nullptr;
}

void NamesOldLoader::load(const QString& skyCultureDir, const bool convertUntranslatableNamesToNative)
{
	loadStarNames(skyCultureDir, convertUntranslatableNamesToNative);
	loadDSONames(skyCultureDir, convertUntranslatableNamesToNative);
	loadPlanetNames(skyCultureDir);
}

bool NamesOldLoader::dumpJSON(std::ostream& s) const
{
	if (starNames.isEmpty() && dsoNames.isEmpty() && planetNames.isEmpty())
		return false;
	s << "  \"common_names\": {\n";
	const auto starKeys = starNames.keys();
	for(int k = 0; k < starKeys.size(); ++k)
	{
		const auto& key = starKeys[k];
		const auto prefix = "    \"HIP " + std::to_string(key) + "\": [";
		s << prefix;
		const auto& values = starNames[key];
		for(unsigned v = 0; v < values.size(); ++v)
		{
			if(v > 0) s << std::string(prefix.size(), ' ');
			const auto refs = values[v].references.empty() ? "" :
				", \"references\": [" + formatReferences(values[v].references) + "]";
			auto xcomments = values[v].translatorsComments;
			if(!xcomments.isEmpty())
				xcomments = ", \"translators_comments\": \"" + jsonEscape(xcomments.trimmed()) + "\"";
			if(values[v].englishName.isEmpty())
				s << ("{\"native\": \"" + values[v].nativeName + '"' + refs + xcomments + "}").toStdString();
			else if(values[v].nativeName.isEmpty())
				s << ("{\"english\": \"" + values[v].englishName + '"' + refs + xcomments + "}").toStdString();
			else
				s << ("{\"english\": \"" + values[v].englishName + "\", \"native\": \"" + values[v].nativeName +'"' + refs + xcomments + "}").toStdString();
			if(v+1 != values.size()) s << ",\n";
		}
		if(k+1 != starKeys.size() || !dsoNames.isEmpty() || !planetNames.isEmpty())
			s << "],\n";
		else
			s << "]\n";
	}

	const auto dsoKeys = dsoNames.keys();
	for(int k = 0; k < dsoKeys.size(); ++k)
	{
		const auto& key = dsoKeys[k];
		const auto prefix = ("    \"" + key + "\": [").toStdString();
		s << prefix;
		const auto& values = dsoNames[key];
		for(unsigned v = 0; v < values.size(); ++v)
		{
			if(v > 0) s << std::string(prefix.size(), ' ');
			const auto refs = values[v].references.empty() ? "" :
				", \"references\": [" + formatReferences(values[v].references) + "]";
			auto xcomments = values[v].translatorsComments;
			if(!xcomments.isEmpty())
				xcomments = ", \"translators_comments\": \"" + jsonEscape(xcomments.trimmed()) + "\"";
			if(values[v].englishName.isEmpty())
				s << ("{\"native\": \"" + values[v].nativeName + '"' + refs + xcomments + "}").toStdString();
			else if(values[v].nativeName.isEmpty())
				s << ("{\"english\": \"" + values[v].englishName + '"' + refs + xcomments + "}").toStdString();
			else
				s << ("{\"english\": \"" + values[v].englishName + "\", \"native\": \"" + values[v].nativeName +'"' + refs + xcomments + "}").toStdString();
			if(v+1 != values.size()) s << ",\n";
		}
		if(k+1 != dsoKeys.size() || !planetNames.isEmpty())
			s << "],\n";
		else
			s << "]\n";
	}

	const auto planetKeys = planetNames.keys();
	for(int k = 0; k < planetKeys.size(); ++k)
	{
		const auto& key = planetKeys[k];
		const auto prefix = ("    \"NAME " + key + "\": [").toStdString();
		s << prefix;
		const auto& values = planetNames[key];
		for(unsigned v = 0; v < values.size(); ++v)
		{
			if(v > 0) s << std::string(prefix.size(), ' ');
			auto xcomments = values[v].translatorsComments;
			if(!xcomments.isEmpty())
				xcomments = ", \"translators_comments\": \"" + jsonEscape(xcomments.trimmed()) + "\"";
			s << ("{\"english\": \"" + values[v].english +
			   "\", \"native\": \"" + values[v].native + "\"" + xcomments + "}").toStdString();
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
