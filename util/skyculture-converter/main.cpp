#include <sstream>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QRegularExpression>
#include "NamesOldLoader.hpp"
#include "AsterismOldLoader.hpp"
#include "DescriptionOldLoader.hpp"
#include "ConstellationOldLoader.hpp"

QString convertLicense(const QString& license)
{
	auto parts = license.split("+");
	for(auto& p : parts)
		p = p.simplified();
	for(auto& lic : parts)
	{
		if(lic.startsWith("Free Art ")) continue;

		lic.replace(QRegularExpression("(?: International)?(?: Publice?)? License"), "");
	}

	if(parts.size() == 1) return parts[0];
	if(parts.size() == 2)
	{
		if(parts[1].startsWith("Free Art ") && !parts[0].startsWith("Free Art "))
			return "Text and data: " + parts[0] + "\n\nIllustrations: " + parts[1];
		else if(parts[0].startsWith("Free Art ") && !parts[1].startsWith("Free Art "))
			return "Text and data: " + parts[1] + "\n\nIllustrations: " + parts[0];
	}
	std::cerr << "Unexpected combination of licenses, leaving them unformatted.\n";
	return license;
}

void convertInfoIni(const QString& dir, std::ostream& s, QString& boundariesType, QString& author, QString& credit, QString& license)
{
	QSettings pd(dir + "/info.ini", QSettings::IniFormat); // FIXME: do we really need StelIniFormat here instead?
	const auto englishName = pd.value("info/name").toString();
	author = pd.value("info/author").toString();
	credit = pd.value("info/credit").toString();
	license = pd.value("info/license", "").toString();
	const auto region = pd.value("info/region", "???").toString();
	const auto classification = pd.value("info/classification").toString();
	boundariesType = pd.value("info/boundaries", "none").toString();

	const std::string highlight = "???";

	// Now emit the JSON snippet
	s << "{\n"
	   "  \"id\": \"" + QFileInfo(dir).fileName().toStdString() + "\",\n"
	   "  \"region\": \"" + region.toStdString() + "\",\n"
	   "  \"classification\": [\"" + classification.toStdString() + "\"],\n"
	   "  \"fallback_to_international_names\": false,\n"
	   "  \"thumbnail\": \"???\",\n"
	   "  \"thumbnail_bscale\": 2,\n"
	   "  \"highlight\": \"" + highlight + "\",\n";
}

void writeEnding(std::string& s)
{
	if(s.size() > 2 && s.substr(s.size() - 2) == ",\n")
		s.resize(s.size() - 2);
	s += "\n}\n";
}

int main(int argc, char** argv)
{
	if (argc != 3)
	{
		std::cerr << "Usage: " << argv[0] << " skyCultureDir outputDir\n";
		return 1;
	}

	QString inDir = argv[1];
	QString outDir = argv[2];
	if(QFile(outDir).exists())
	{
		std::cerr << "Output directory already exists, won't touch it.\n";
		return 1;
	}
	if (!QFile(inDir+"/info.ini").exists())
	{
		std::cerr << "Error: info.ini file wasn't found\n";
		return 1;
	}
	std::stringstream out;
	QString boundariesType, author, credit, license;
	convertInfoIni(inDir, out, boundariesType, author, credit, license);

	AsterismOldLoader aLoader;
	aLoader.load(inDir);

	ConstellationOldLoader cLoader;
	if(boundariesType.toLower() != "none")
		cLoader.setBoundariesType(boundariesType.toStdString());
	cLoader.load(inDir, outDir);

	NamesOldLoader nLoader;
	nLoader.load(inDir);

	std::cerr << "Starting emission of JSON...\n\n";
	aLoader.dumpJSON(out);
	cLoader.dumpJSON(out);
	nLoader.dumpJSON(out);

	auto str = std::move(out).str();
	writeEnding(str);

	if(!QDir().mkpath(outDir))
	{
		std::cerr << "Failed to create output directory\n";
		return 1;
	}
	{
		std::ofstream outFile((outDir+"/index.json").toStdString());
		outFile << str;
		outFile.flush();
		if(!outFile)
		{
			std::cerr << "Failed to write index.json\n";
			return 1;
		}
	}

	DescriptionOldLoader dLoader;
	license = convertLicense(license);
	dLoader.load(inDir, author, credit, license);
	dLoader.dump(outDir);

	std::cerr << "--- NOTE ---\n";
	std::cerr << "* Some JSON values can't be deduced from the old-format data. They have been"
	             " marked by \"???\". Please replace them with something sensible.\n";
	std::cerr << "* Also, langs_use_native_names key is omitted since it has no counterpart"
	             " in the old format. If this sky culture needs it, please add it manually.\n";
	std::cerr << "* The transformation of the description text is very basic, please check that"
	             " it looks as it should. Pay special attention at References, Authors, and"
	             " License sections, which may have been formulated in a suboptimal way.\n";
}
