#include "ConstellationOldLoader.hpp"
#include <cmath>
#include <iomanip>
#include <QDir>
#include <QFile>
#include <QDebug>
#include <QImage>
#include <QFileInfo>
#include <QRegularExpression>
#include "Utils.hpp"

bool ConstellationOldLoader::Constellation::read(QString const& record)
{
	unsigned int HP;

	abbreviation.clear();
	unsigned numberOfSegments = 0;

	QString buf(record);
	QTextStream istr(&buf, QIODevice::ReadOnly);
	// allow mixed-case abbreviations now that they can be displayed on screen. We then need toUpper() in comparisons.
	istr >> abbreviation >> numberOfSegments;
	if (istr.status()!=QTextStream::Ok)
		return false;

	points.clear();
	points.reserve(numberOfSegments*2);
	for (unsigned i = 0; i < numberOfSegments*2; ++i)
	{
		HP = 0;
		istr >> HP;
		if(HP == 0) return false;
		points.push_back(HP);
	}

	return true;
}

void ConstellationOldLoader::loadSeasonalRules(const QString& rulesFile)
{
	// Constellation not loaded yet
	if (constellations.empty()) return;

	if (!QFileInfo(rulesFile).exists())
	{
		// Current starlore didn't support the seasonal rules
		return;
	}

	// Open file
	QFile seasonalRulesFile(rulesFile);
	if (!seasonalRulesFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qDebug() << "Cannot open file" << QDir::toNativeSeparators(rulesFile);
		return;
	}

	// Now parse the file
	// lines to ignore which start with a # or are empty
	static const QRegularExpression commentRx("^(\\s*#.*|\\s*)$");

	// lines which look like records - we use the RE to extract the fields
	// which will be available in recRx.capturedTexts()
	static const QRegularExpression recRx("^\\s*(\\w+)\\s+(\\w+)\\s+(\\w+)\\n");

	// Some more variables to use in the parsing
	Constellation *aster;
	QString record, shortName;

	// keep track of how many records we processed.
	int totalRecords=0;
	int readOk=0;
	int lineNumber=0;
	while (!seasonalRulesFile.atEnd())
	{
		record = QString::fromUtf8(seasonalRulesFile.readLine());
		lineNumber++;

		// Skip comments
		if (commentRx.match(record).hasMatch())
			continue;

		totalRecords++;

		QRegularExpressionMatch recMatch=recRx.match(record);
		if (!recMatch.hasMatch())
		{
			qWarning() << "ERROR - cannot parse record at line" << lineNumber << "in seasonal rules file" << QDir::toNativeSeparators(rulesFile);
		}
		else
		{
			shortName = recMatch.captured(1);
			aster = findFromAbbreviation(shortName);
			// If the constellation exists, set the rule
			if (aster != Q_NULLPTR)
			{
				aster->beginSeason = recMatch.captured(2).toInt();
				aster->endSeason = recMatch.captured(3).toInt();
				aster->seasonalRuleEnabled = true;
				readOk++;
			}
			else
			{
				qWarning() << "WARNING - constellation abbreviation" << shortName << "not found when loading seasonal rules for constellations";
			}
		}
	}
	seasonalRulesFile.close();
	qDebug() << "Loaded" << readOk << "/" << totalRecords << "seasonal rules";
}
auto ConstellationOldLoader::findFromAbbreviation(const QString& abbrev) -> Constellation*
{
	for(auto& cons : constellations)
		if(cons.abbreviation == abbrev)
			return &cons;
	return nullptr;
}

void ConstellationOldLoader::loadLinesAndArt(const QString& skyCultureDir, const QString& outDir)
{
	const auto fileName = skyCultureDir+"/constellationship.fab";
	const auto artfileName = skyCultureDir+"/constellationsart.fab";
	QFile in(fileName);
	if (!in.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qWarning() << "Can't open constellation data file" << QDir::toNativeSeparators(fileName);
		Q_ASSERT(0);
	}

	int totalRecords=0;
	QString record;
	static const QRegularExpression commentRx("^(\\s*#.*|\\s*)$"); // pure comment lines or empty lines
	while (!in.atEnd())
	{
		record = QString::fromUtf8(in.readLine());
		if (!commentRx.match(record).hasMatch())
			totalRecords++;
	}
	in.seek(0);

	constellations.clear();
	Constellation* cons = nullptr;

	// read the file of line patterns, adding a record per non-comment line
	int currentLineNumber = 0;	// line in file
	int readOk = 0;			// count of records processed OK
	while (!in.atEnd())
	{
		record = QString::fromUtf8(in.readLine());
		currentLineNumber++;
		if (commentRx.match(record).hasMatch())
			continue;

		constellations.push_back({});
		auto* cons = &constellations.back();
		if(cons->read(record))
		{
			++readOk;
		}
		else
		{
			qWarning() << "ERROR reading constellation lines record at line " << currentLineNumber;
			constellations.pop_back();
		}
	}
	in.close();
	if(readOk != totalRecords)
		qDebug() << "Loaded" << readOk << "/" << totalRecords << "constellation records successfully";

	// It's possible to have no art - just constellations
	if (artfileName.isNull() || artfileName.isEmpty())
		return;
	QFile fic(artfileName);
	if (!fic.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qWarning() << "Can't open constellation art file" << QDir::toNativeSeparators(fileName);
		return;
	}

	totalRecords=0;
	while (!fic.atEnd())
	{
		record = QString::fromUtf8(fic.readLine());
		if (!commentRx.match(record).hasMatch())
			totalRecords++;
	}
	fic.seek(0);

	// Read the constellation art file with the following format :
	// ShortName texture_file x1 y1 hp1 x2 y2 hp2
	// Where :
	// shortname is the international short name (i.e "Lep" for Lepus)
	// texture_file is the graphic file of the art texture
	// x1 y1 are the x and y texture coordinates in pixels of the star of hipparcos number hp1
	// x2 y2 are the x and y texture coordinates in pixels of the star of hipparcos number hp2
	// The coordinate are taken with (0,0) at the top left corner of the image file
	QString shortname;
	QString texfile;
	unsigned int x1, y1, x2, y2, x3, y3, hp1, hp2, hp3;

	currentLineNumber = 0;	// line in file
	readOk = 0;		// count of records processed OK

	while (!fic.atEnd())
	{
		++currentLineNumber;
		record = QString::fromUtf8(fic.readLine());
		if (commentRx.match(record).hasMatch())
			continue;

		// prevent leading zeros on numbers from being interpreted as octal numbers
		record.replace(" 0", " ");
		QTextStream rStr(&record);
		rStr >> shortname >> texfile >> x1 >> y1 >> hp1 >> x2 >> y2 >> hp2 >> x3 >> y3 >> hp3;
		if (rStr.status()!=QTextStream::Ok)
		{
			qWarning() << "ERROR parsing constellation art record at line" << currentLineNumber << "of art file";
			continue;
		}

		cons = findFromAbbreviation(shortname);
		if (!cons)
		{
			qWarning() << "ERROR in constellation art file at line" << currentLineNumber
					   << "constellation" << shortname << "unknown";
		}
		else
		{
			cons->artTexture = "illustrations/" + texfile;
			const auto texPath = skyCultureDir+"/"+texfile;
			QImage tex(texPath);
			if(tex.isNull())
			{
				std::cerr << "Error: failed to open texture file \"" << texPath.toStdString() << "\"\n";
			}
			else
			{
				cons->textureSize = tex.size();

				const auto targetPath = outDir+"/"+cons->artTexture;
				if(!QDir().mkpath(QFileInfo(targetPath).absoluteDir().path()))
				{
					qCritical() << "Failed to create output directory for texture file" << targetPath;
					continue;
				}

				const auto texInfo = QFileInfo(texPath);
				const auto targetInfo = QFileInfo(targetPath);
				bool targetExistsAndDiffers = targetInfo.exists() && texInfo.size() != targetInfo.size();
				if(targetInfo.exists() && !targetExistsAndDiffers)
				{
					// Check that the contents are the same
					QFile in(texPath);
					if(!in.open(QFile::ReadOnly))
					{
						qCritical().noquote() << "Failed to open file" << texPath;
						continue;
					}
					QFile out(targetPath);
					if(!out.open(QFile::ReadOnly))
					{
						qCritical().noquote() << "Failed to open file" << targetPath;
						continue;
					}
					targetExistsAndDiffers = in.readAll() != out.readAll();
				}
				if(targetExistsAndDiffers)
				{
					qCritical() << "Image file names collide:" << texPath << "and" << targetPath;
					continue;
				}

				// If it exists, it is the same as the file we're trying to copy, so don't copy it in that case
				if(!targetInfo.exists())
				{
					QFile file(texPath);
					if(!file.copy(targetPath))
					{
						std::cerr << "Error: failed to copy texture file \"" << texPath.toStdString()
						          << "\" to \"" << targetPath.toStdString() << "\""
						          << ": " << file.errorString().toStdString() << "\n";
					}
				}
			}

			cons->artP1.x = x1;
			cons->artP1.y = y1;
			cons->artP1.hip = hp1;

			cons->artP2.x = x2;
			cons->artP2.y = y2;
			cons->artP2.hip = hp2;

			cons->artP3.x = x3;
			cons->artP3.y = y3;
			cons->artP3.hip = hp3;

			++readOk;
		}
	}

	if(readOk != totalRecords)
		qDebug() << "Loaded" << readOk << "/" << totalRecords << "constellation art records successfully";
	fic.close();
}

void ConstellationOldLoader::loadNames(const QString& skyCultureDir)
{
	const auto namesFile = skyCultureDir + "/constellation_names.eng.fab";

	// Constellation not loaded yet
	if (constellations.empty()) return;

	// clear previous names
	for (auto& constellation : constellations)
	{
		constellation.englishName.clear();
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

	// lines which look like records - we use the RE to extract the fields
	// which will be available in recRx.capturedTexts()
	// abbreviation is allowed to start with a dot to mark as "hidden".
	static const QRegularExpression recRx("^\\s*(\\.?\\S+)\\s+\"(.*)\"\\s+_[(]\"(.*)\"[)]\\s*([\\,\\d\\s]*)\\n");
	static const QRegularExpression ctxRx("(.*)\",\\s*\"(.*)");

	// keep track of how many records we processed.
	int totalRecords=0;
	int readOk=0;
	int lineNumber=0;
	QString translatorsComments;
	while (!commonNameFile.atEnd())
	{
		QString record = QString::fromUtf8(commonNameFile.readLine());
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
			qWarning() << "ERROR - cannot parse record at line" << lineNumber << "in constellation names file" << QDir::toNativeSeparators(namesFile) << ":" << record;
		}
		else
		{
			QString shortName = recMatch.captured(1);
			Constellation *aster = findFromAbbreviation(shortName);
			// If the constellation exists, set the English name
			if (aster)
			{
				aster->nativeName = recMatch.captured(2).trimmed();
				QString ctxt = recMatch.captured(3);
				QRegularExpressionMatch ctxMatch=ctxRx.match(ctxt);
				if (ctxMatch.hasMatch())
				{
					aster->englishName = ctxMatch.captured(1);
					//aster->context = ctxMatch.captured(2);
				}
				else
				{
					aster->englishName = ctxt;
					//aster->context = "";
				}
				aster->translatorsComments = translatorsComments;
				aster->references = parseReferences(recMatch.captured(4).trimmed());
				readOk++;
			}
			else
			{
				qWarning() << "WARNING - constellation abbreviation" << shortName << "not found when loading constellation names";
			}
		}
		translatorsComments = "";
	}
	commonNameFile.close();
	if(readOk != totalRecords)
		qDebug() << "Loaded" << readOk << "/" << totalRecords << "constellation names";
}

void ConstellationOldLoader::loadBoundaries(const QString& skyCultureDir)
{
	if(QString(boundariesType.c_str()).toLower() == "none")
		return;

	const bool ownB = QString(boundariesType.c_str()).toLower() == "own";
	const auto boundaryFile = ownB ? skyCultureDir + "/constellation_boundaries.dat"
	                               : skyCultureDir + "/../../data/constellation_boundaries.dat";

	// Modified boundary file by Torsten Bronger with permission
	// http://pp3.sourceforge.net
	QFile dataFile(boundaryFile);
	if (!dataFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qWarning() << "Boundary file" << QDir::toNativeSeparators(boundaryFile) << "not found";
		return;
	}

	QString data = "";

	// Added support of comments for constellation_boundaries.dat file
	static const QRegularExpression commentRx("^(\\s*#.*|\\s*)$");
	while (!dataFile.atEnd())
	{
		// Read the line
		QString record = QString::fromUtf8(dataFile.readLine());

		// Skip comments
		if (commentRx.match(record).hasMatch())
			continue;

		// Append the data
		data.append(record);
	}

	boundaries.clear();
	// Read and parse the data without comments
	QTextStream istr(&data);
	unsigned int i = 0;
	while (!istr.atEnd())
	{
		unsigned num = 0;
		istr >> num;
		if(num == 0)
			continue; // empty line

		boundaries.push_back({});
		auto& line = boundaries.back();
		auto& points = line.points;

		for (unsigned int j=0;j<num;j++)
		{
			double DE, RA;
			istr >> RA >> DE;
			points.emplace_back(RaDec{RA,DE});
		}

		unsigned numc;
		istr >> numc;
		if(numc != 2)
		{
			std::cerr << "Error: expected 2 constellations per boundary, got " << numc << "\n";
			boundaries.clear();
			return;
		}

		istr >> line.cons1;
		istr >> line.cons2;
		if(line.cons1 == "SER1" || line.cons1 == "SER2") line.cons1 = "SER";
		if(line.cons2 == "SER1" || line.cons2 == "SER2") line.cons2 = "SER";
		i++;
	}
	qDebug() << "Loaded" << i << "constellation boundary segments";
}

void ConstellationOldLoader::load(const QString& skyCultureDir, const QString& outDir)
{
	skyCultureName = QFileInfo(skyCultureDir).fileName();
	loadLinesAndArt(skyCultureDir, outDir);
	loadNames(skyCultureDir);

	for(const auto& cons : constellations)
	{
		if(cons.artTexture.isEmpty())
		{
			//std::cerr << "No texture found for constellation " << cons.englishName.toStdString() << " (" << cons.abbreviation.toStdString() << ")\n";
			continue;
		}
		if(cons.textureSize.width() <= 0 || cons.textureSize.height() <= 0)
		{
			std::cerr << "Failed to find texture size for constellation " << cons.englishName.toStdString() << " (" << cons.abbreviation.toStdString() << ")\n";
			continue;
		}
	}

	loadBoundaries(skyCultureDir);
	loadSeasonalRules(skyCultureDir + "/seasonal_rules.fab");
}

auto ConstellationOldLoader::find(QString const& englishName) const -> const Constellation*
{
	for(const auto& cons : constellations)
		if(cons.englishName == englishName)
			return &cons;
	return nullptr;
}

bool ConstellationOldLoader::dumpConstellationsJSON(std::ostream& s) const
{
	if(constellations.empty()) return false;

	s << "  \"constellations\": [\n";
	for(const auto& c : constellations)
	{
		using std::to_string;
		s << "    {\n"
		     "      \"id\": \"CON "+skyCultureName.toStdString()+" "+c.abbreviation.toStdString()+"\",\n";

		s << "      \"lines\": [";
		auto& points = c.points;
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
		s << "],\n";

		if(!c.artTexture.isEmpty())
		{
			s << "      \"image\": {\n"
			     "        \"file\": \"" << c.artTexture.toStdString() << "\",\n"
			     "        \"size\": [" << to_string(c.textureSize.width()) << ", " << to_string(c.textureSize.height()) << "],\n"
			     "        \"anchors\": [\n"
			     "          {\"pos\": [" << to_string(c.artP1.x) << ", " << to_string(c.artP1.y) << "], \"hip\": " << to_string(c.artP1.hip) << "},\n"
			     "          {\"pos\": [" << to_string(c.artP2.x) << ", " << to_string(c.artP2.y) << "], \"hip\": " << to_string(c.artP2.hip) << "},\n"
			     "          {\"pos\": [" << to_string(c.artP3.x) << ", " << to_string(c.artP3.y) << "], \"hip\": " << to_string(c.artP3.hip) << "}\n"
			     "        ]\n"
			     "      },\n";
		}

		if(c.seasonalRuleEnabled)
		{
			s << "      \"visibility\": {\"months\": [" << c.beginSeason << ", " << c.endSeason << "]},\n";
		}

		auto refs = formatReferences(c.references).toStdString();
		if(!refs.empty()) refs = ", \"references\": [" + refs + "]";

		auto xcomments = c.translatorsComments;
		if(!xcomments.isEmpty())
			xcomments = ", \"translators_comments\": \"" + jsonEscape(xcomments.trimmed()) + "\"";

		s << "      \"common_name\": {\"english\": \"" << c.englishName.toStdString()
		  << (c.nativeName.isEmpty() ? "\"" : "\", \"native\": \"" + c.nativeName.toStdString() + "\"")
		  << refs << xcomments.toStdString() << "}\n"
		     "    }";
		if(&c != &constellations.back())
			s << ",\n";
		else
			s << "\n";
	}
	s << "  ],\n";

	return true;
}

bool ConstellationOldLoader::dumpBoundariesJSON(std::ostream& s) const
{
	if(boundaries.empty()) return false;

	s << "  \"edges_type\": \"" << boundariesType << "\",\n";
	s << "  \"edges\": [\n";
	s.fill('0');
#define W2 std::setw(2)
	for(const auto& line : boundaries)
	{
		for(unsigned n = 0; n < line.points.size() - 1; ++n)
		{
			const auto& p1 = line.points[n];
			const auto& p2 = line.points[n+1];

			const int ra1ss = std::lround(3600*p1.ra);
			const int ra1h = ra1ss / 3600;
			const int ra1m = ra1ss / 60 % 60;
			const int ra1s = ra1ss % 60;

			const int ra2ss = std::lround(3600*p2.ra);
			const int ra2h = ra2ss / 3600;
			const int ra2m = ra2ss / 60 % 60;
			const int ra2s = ra2ss % 60;

			const int de1ss = std::lround(std::abs(3600*p1.dec));
			const int de1d = de1ss / 3600;
			const int de1m = de1ss / 60 % 60;
			const int de1s = de1ss % 60;

			const int de2ss = std::lround(std::abs(3600*p2.dec));
			const int de2d = de2ss / 3600;
			const int de2m = de2ss / 60 % 60;
			const int de2s = de2ss % 60;

			s << "    \"___:___ __ "
			  << W2 << ra1h << ":" << W2 << ra1m << ":" << W2 << ra1s << " "
			  << (p1.dec>0 ? '+' : '-') << W2 << de1d << ":" << W2 << de1m << ":" << W2 << de1s << " "
			  << W2 << ra2h << ":" << W2 << ra2m << ":" << W2 << ra2s << " "
			  << (p2.dec>0 ? '+' : '-') << W2 << de2d << ":" << W2 << de2m << ":" << W2 << de2s << " "
			  << line.cons1.toStdString() << " " << line.cons2.toStdString() << "\"";

			if(n+2 == line.points.size() && &line == &boundaries.back())
				s << "\n";
			else
				s << ",\n";
		}
	}
#undef W
	s << "  ],\n";

	return true;
}

bool ConstellationOldLoader::dumpJSON(std::ostream& s) const
{
	return dumpConstellationsJSON(s) &&
	       dumpBoundariesJSON(s);
}
