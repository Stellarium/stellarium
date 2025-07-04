#include "ScmSkyCulture.hpp"
#include "types/Classification.hpp"
#include <utility>
#include <QFile>

void scm::ScmSkyCulture::setId(const QString &id)
{
	ScmSkyCulture::id = id;
}

void scm::ScmSkyCulture::setFallbackToInternationalNames(bool fallback)
{
	ScmSkyCulture::fallbackToInternationalNames = fallback;
}

void scm::ScmSkyCulture::addAsterism(const scm::ScmAsterism &asterism)
{
	asterisms.push_back(asterism);
}

void scm::ScmSkyCulture::removeAsterism(const QString &id)
{
	asterisms.erase(remove_if(begin(asterisms), end(asterisms),
	                          [id](scm::ScmAsterism const &a) { return a.getId() == id; }),
	                end(asterisms));
}

scm::ScmConstellation &scm::ScmSkyCulture::addConstellation(const QString &id,
                                                            const std::vector<CoordinateLine> &coordinates,
                                                            const std::vector<StarLine> &stars)
{
	scm::ScmConstellation constellationObj(coordinates, stars);
	constellationObj.setId(id);
	constellations.push_back(std::move(constellationObj));
	return constellations.back();
}

void scm::ScmSkyCulture::removeConstellation(const QString &id)
{
	constellations.erase(remove_if(begin(constellations), end(constellations),
	                               [id](ScmConstellation const &c) { return c.getId() == id; }),
	                     end(constellations));
}

scm::ScmConstellation *scm::ScmSkyCulture::getConstellation(const QString &id)
{
	for (auto &constellation : constellations)
	{
		if (constellation.getId() == id) return &constellation;
	}
	return nullptr;
}

std::vector<scm::ScmConstellation> *scm::ScmSkyCulture::getConstellations()
{
	return &constellations;
}

QJsonObject scm::ScmSkyCulture::toJson() const
{
	QJsonObject scJsonObj;
	scJsonObj["id"]     = id;
	scJsonObj["region"] = description.geoRegion;
	// for some reason, the classification is inside an array, eg. ["historical"]
	QJsonArray classificationArray = QJsonArray::fromStringList(
		QStringList() << classificationTypeToString(description.classification));
	scJsonObj["classification"]                  = classificationArray;
	scJsonObj["fallback_to_international_names"] = fallbackToInternationalNames;
	QJsonArray constellationsArray;
	for (const auto &constellation : constellations)
	{
		constellationsArray.append(constellation.toJson(id));
	}
	scJsonObj["constellations"] = constellationsArray;

	// TODO: Add asterisms to the JSON object (currently out of scope)
	// TODO: Add common names to the JSON object (currently out of scope)

	return scJsonObj;
}

void scm::ScmSkyCulture::draw(StelCore *core) const
{
	for (auto &constellation : constellations)
	{
		constellation.drawConstellation(core);
	}
}

void scm::ScmSkyCulture::setDescription(const scm::Description &description)
{
	ScmSkyCulture::description = description;
}

bool scm::ScmSkyCulture::saveDescriptionAsMarkdown(QFile &file)
{
	if (file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		const scm::Description &desc = ScmSkyCulture::description;
		const scm::License license   = scm::LICENSES.at(desc.license);

		QTextStream out(&file);
		out << "# " << desc.name << "\n\n";
		out << "## Authors\n" << desc.authors << "\n\n";
		out << "## License\n### " << license.name << "\n" << license.description << "\n\n";
		out << "## Culture Description\n" << desc.cultureDescription << "\n\n";
		out << "## About\n" << desc.about << "\n\n";

		out << "## Geographical Region\n" << desc.geoRegion << "\n\n";
		out << "## Sky\n" << desc.sky << "\n\n";
		out << "## Moon and Sun\n" << desc.moonAndSun << "\n\n";
		out << "## Planets\n" << desc.planets << "\n\n";
		out << "## Zodiac\n" << desc.zodiac << "\n\n";
		out << "## Milky Way\n" << desc.milkyWay << "\n\n";
		out << "## Other Celestial Objects\n" << desc.otherObjects << "\n\n";

		out << "## Constellations\n" << desc.constellations << "\n\n";
		out << "## References\n" << desc.references << "\n\n";
		out << "## Acknowledgements\n" << desc.acknowledgements << "\n\n";
		out << "## Classification\n " << classificationTypeToString(desc.classification) << "\n\n";

		try
		{
			file.close();
			return true; // successfully saved
		}
		catch (const std::exception &e)
		{
			qWarning("Error closing file: %s", e.what());
			return false; // error occurred while closing the file
		}
	}
	else
	{
		qWarning("Could not open file for writing: %s", qPrintable(file.fileName()));
		return false; // file could not be opened
	}
}

bool scm::ScmSkyCulture::saveIllustrations(const QString &directory)
{
	bool success = true;
	for (auto &constellation : constellations)
	{
		success &= constellation.saveArtwork(directory);
	}

	return success;
}

const QString &scm::ScmSkyCulture::getId() const
{
	return id;
}
