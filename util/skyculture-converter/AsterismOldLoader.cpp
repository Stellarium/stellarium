#include <cmath>
#include <limits>
#include <ostream>
#include <QDir>
#include <QFile>
#include <QDebug>
#include <QRegularExpression>

#include "AsterismOldLoader.hpp"
#include "Utils.hpp"

std::ostream& operator<<(std::ostream& s, const AsterismOldLoader::Asterism::Star& star)
{
	if (star.HIP > 0)
		s << star.HIP;
	else
		s << "[" << star.RA << ", " << star.DE << "]";
	return s;
}

bool AsterismOldLoader::Asterism::read(const QString& record)
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

auto AsterismOldLoader::findFromAbbreviation(const QString& abbrev) const -> Asterism*
{
	for (const auto asterism : asterisms)
		if (asterism->abbreviation == abbrev)
			return asterism;
	return nullptr;
}

void AsterismOldLoader::load(const QString& skyCultureDir, const QString& cultureId)
{
	this->cultureId = cultureId;
	QString fic = skyCultureDir+"/asterism_lines.fab";
	if (QFileInfo(fic).exists())
	{
		hasAsterism = true;
		loadLines(fic);
	}
	else
	{
		hasAsterism = false;
		qWarning() << "No asterisms in " << skyCultureDir;
	}

	// load asterism names
	fic = skyCultureDir + "/asterism_names.eng.fab";
	if (QFileInfo(fic).exists())
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
				aster->translatorsComments = translatorsComments;
				aster->references = parseReferences(recMatch.captured(3).trimmed());
				readOk++;
			}
			else
			{
				qWarning() << "WARNING - asterism abbreviation" << shortName << "not found when loading asterism names";
			}
		}
		translatorsComments = "";
	}
	commonNameFile.close();
	qDebug() << "Loaded" << readOk << "/" << totalRecords << "asterism names";
}

auto AsterismOldLoader::find(QString const& englishName) const -> const Asterism*
{
	for(const auto*const aster : asterisms)
		if(aster->englishName == englishName)
			return aster;
	return nullptr;
}

bool AsterismOldLoader::dumpJSON(std::ostream& s) const
{
	if (!hasAsterism) return false;

	s << "  \"asterisms\": [\n";
	for (const Asterism*const ast : asterisms)
	{
		s << "    {\n";
		s << "      \"id\": \"AST " << cultureId.toStdString() << " " << ast->abbreviation.toStdString() << "\",\n";
		if (!ast->englishName.isEmpty())
		{
			auto refs = formatReferences(ast->references).toStdString();
			if(!refs.empty()) refs = ", \"references\": [" + refs + "]";

			auto xcomments = ast->translatorsComments;
			if(!xcomments.isEmpty())
				xcomments = ", \"translators_comments\": \"" + jsonEscape(xcomments.trimmed()) + "\"";

			s << "      \"common_name\": {\"english\": \"" << ast->englishName.toStdString() << "\"" << refs << xcomments.toStdString() << "},\n";
		}
		const bool isRayHelper = ast->typeOfAsterism == 0;
		if(isRayHelper)
			s << "      \"is_ray_helper\": true,\n";

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
