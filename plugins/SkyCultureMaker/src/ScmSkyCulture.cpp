/*
 * Sky Culture Maker plug-in for Stellarium
 *
 * Copyright (C) 2025 Vincent Gerlach
 * Copyright (C) 2025 Luca-Philipp Grumbach
 * Copyright (C) 2025 Fabian Hofer
 * Copyright (C) 2025 Mher Mnatsakanyan
 * Copyright (C) 2025 Richard Hofmann
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ScmSkyCulture.hpp"
#include "types/Classification.hpp"
#include <utility>
#include <QFile>
#include <QTextStream>

void scm::ScmSkyCulture::setId(const QString &id)
{
	ScmSkyCulture::id = id;
}

void scm::ScmSkyCulture::setFallbackToInternationalNames(bool fallback)
{
	ScmSkyCulture::fallbackToInternationalNames = fallback;
}

scm::ScmConstellation &scm::ScmSkyCulture::addConstellation(const QString &id,
                                                            const std::vector<ConstellationLine> &lines,
                                                            const bool isDarkConstellation)
{
	constellations.push_back(std::make_unique<scm::ScmConstellation>(id, lines, isDarkConstellation));
	return *constellations.back();
}

void scm::ScmSkyCulture::removeConstellation(const QString &id)
{
	constellations.erase(remove_if(begin(constellations), end(constellations),
	                               [id](const std::unique_ptr<ScmConstellation> &c) { return c->getId() == id; }),
	                     end(constellations));
}

scm::ScmConstellation *scm::ScmSkyCulture::getConstellation(const QString &id)
{
	auto it = std::find_if(constellations.begin(), constellations.end(),
						   [&id](const std::unique_ptr<ScmConstellation> &c) { return c->getId() == id; });
	return it != constellations.end() ? it->get() : nullptr;
}

std::vector<std::unique_ptr<scm::ScmConstellation>> *scm::ScmSkyCulture::getConstellations()
{
	return &constellations;
}

QJsonObject scm::ScmSkyCulture::toJson(const bool mergeLines) const
{
	QJsonObject scJsonObj;
	scJsonObj["id"] = id;
	// for some reason, the classification is inside an array, eg. ["historical"]
	QJsonArray classificationArray = QJsonArray::fromStringList(
		QStringList() << classificationTypeToString(description.classification));
	scJsonObj["classification"]                  = classificationArray;
	scJsonObj["fallback_to_international_names"] = fallbackToInternationalNames;
	QJsonArray constellationsArray;
	for (const auto &constellation : constellations)
	{
		constellationsArray.append(constellation->toJson(id, mergeLines));
	}
	scJsonObj["constellations"] = constellationsArray;

	return scJsonObj;
}

void scm::ScmSkyCulture::draw(StelCore *core) const
{
	for (auto &constellation : constellations)
	{
		if(!constellation->isHidden)
		{
			constellation->draw(core);
		}
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
		// the sky heading is not only needed for the sky description, but also for the subsections
		const bool hasSkyHeading = !desc.sky.trimmed().isEmpty() || !desc.moonAndSun.trimmed().isEmpty() || !desc.planets.trimmed().isEmpty() ||
		                           !desc.zodiac.trimmed().isEmpty() ||
		                           !desc.milkyWay.trimmed().isEmpty() || !desc.otherObjects.trimmed().isEmpty();

		QTextStream out(&file);
		out << "# " << desc.name << "\n\n";

		out << "## Introduction\n\n" << desc.introduction << "\n\n";

		out << "## Description\n\n" << desc.cultureDescription << "\n\n";

		if (hasSkyHeading)
		{
			out << "### Sky\n\n";
			if (!desc.sky.trimmed().isEmpty())
			{
				out << desc.sky << "\n\n";
			}
			else
			{
				// add a newline if there is a sky heading but no sky description
				out << "\n";
			}
			if (!desc.moonAndSun.trimmed().isEmpty())
			{
				out << "### Moon and Sun\n\n" << desc.moonAndSun << "\n\n";
			}
			if (!desc.planets.trimmed().isEmpty())
			{
				out << "### Planets\n\n" << desc.planets << "\n\n";
			}
			if (!desc.zodiac.trimmed().isEmpty())
			{
				out << "### Zodiac\n\n" << desc.zodiac << "\n\n";
			}
			if (!desc.milkyWay.trimmed().isEmpty())
			{
				out << "### Milky Way\n\n" << desc.milkyWay << "\n\n";
			}
			if (!desc.otherObjects.trimmed().isEmpty())
			{
				out << "### Other Celestial Objects\n\n" << desc.otherObjects << "\n\n";
			}
		}
		if (!desc.constellations.trimmed().isEmpty())
		{
			out << "## Constellations\n\n" << desc.constellations << "\n\n";
		}

		out << "## References\n\n" << desc.references << "\n\n";
		out << "## Authors\n\n" << desc.authors << "\n\n";
		out << "### About\n\n" << desc.about << "\n\n";
		if (!desc.acknowledgements.trimmed().isEmpty())
		{
			out << "### Acknowledgements\n" << desc.acknowledgements << "\n\n";
		}

		out << "## License\n\n" << license.name << "\n\n";

		try
		{
			file.close();
			return true; // successfully saved
		}
		catch (const std::exception &e)
		{
			qWarning("SkyCultureMaker: Error closing file: %s", e.what());
			return false; // error occurred while closing the file
		}
	}
	else
	{
		qWarning("SkyCultureMaker: Could not open file for writing: %s", qPrintable(file.fileName()));
		return false; // file could not be opened
	}
}

bool scm::ScmSkyCulture::saveIllustrations(const QString &directory)
{
	bool success = true;
	for (auto &constellation : constellations)
	{
		success &= constellation->saveArtwork(directory);
	}

	return success;
}

const QString &scm::ScmSkyCulture::getId() const
{
	return id;
}
