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

#include "ScmConstellation.hpp"
#include <QDir>
#include <QFileInfo>

scm::ScmConstellation::ScmConstellation(const QString &id, const std::vector<CoordinateLine> &coordinates,
                                        const std::vector<StarLine> &stars, const bool isDarkConstellation)
	: id(id)
	, coordinates(coordinates)
	, stars(stars)
	, isDarkConstellation(isDarkConstellation)
{
	QSettings *conf = StelApp::getInstance().getSettings();
	constellationNameFont.setPixelSize(conf->value("viewing/constellation_font_size", 15).toInt());

	QString defaultColor          = conf->value("color/default_color", "0.5,0.5,0.7").toString();
	defaultConstellationLineColor = Vec3f(conf->value("color/const_lines_color", defaultColor).toString());
	defaultConstellationNameColor = Vec3f(conf->value("color/const_names_color", defaultColor).toString());

	updateTextPosition();
}

QString scm::ScmConstellation::getId() const
{
	return id;
}

void scm::ScmConstellation::setEnglishName(const QString &name)
{
	englishName = name;
}

QString scm::ScmConstellation::getEnglishName() const
{
	return englishName;
}

void scm::ScmConstellation::setNativeName(const std::optional<QString> &name)
{
	nativeName = name;
}

std::optional<QString> scm::ScmConstellation::getNativeName() const
{
	return nativeName;
}

void scm::ScmConstellation::setPronounce(const std::optional<QString> &pronounce)
{
	ScmConstellation::pronounce = pronounce;
}

std::optional<QString> scm::ScmConstellation::getPronounce() const
{
	return pronounce;
}

void scm::ScmConstellation::setIPA(const std::optional<QString> &ipa)
{
	ScmConstellation::ipa = ipa;
}

std::optional<QString> scm::ScmConstellation::getIPA() const
{
	return ipa;
}

void scm::ScmConstellation::setArtwork(const ScmConstellationArtwork &artwork)
{
	ScmConstellation::artwork = artwork;
}

const scm::ScmConstellationArtwork &scm::ScmConstellation::getArtwork() const
{
	return artwork;
}

void scm::ScmConstellation::setConstellation(const std::vector<CoordinateLine> &coordinates,
                                             const std::vector<StarLine> &stars)
{
	scm::ScmConstellation::coordinates = coordinates;
	scm::ScmConstellation::stars       = stars;

	updateTextPosition();
}

const std::vector<scm::CoordinateLine> &scm::ScmConstellation::getCoordinates() const
{
	return coordinates;
}

const std::vector<scm::StarLine> &scm::ScmConstellation::getStars() const
{
	return stars;
}

void scm::ScmConstellation::drawConstellation(StelCore *core, const Vec3f &lineColor, const Vec3f &nameColor) const
{
	if (isHidden)
	{
		return;
	}
	StelPainter painter(core->getProjection(drawFrame));
	painter.setBlending(true);
	painter.setLineSmooth(true);
	painter.setFont(constellationNameFont);

	painter.setColor(lineColor, 1.0f);

	for (CoordinateLine p : coordinates)
	{
		painter.drawGreatCircleArc(p.start, p.end);
	}

	drawNames(core, painter, nameColor);

	artwork.draw(core, painter);
}

void scm::ScmConstellation::drawConstellation(StelCore *core) const
{
	if (isHidden)
	{
		return;
	}
	drawConstellation(core, defaultConstellationLineColor, defaultConstellationNameColor);
}

void scm::ScmConstellation::drawNames(StelCore *core, StelPainter &sPainter, const Vec3f &nameColor) const
{
	if (isHidden)
	{
		return;
	}

	sPainter.setBlending(true);

	Vec3d velocityObserver(0.);
	if (core->getUseAberration())
	{
		velocityObserver = core->getAberrationVec(core->getJDE());
	}

	Vec3d namePose = XYZname;
	namePose += velocityObserver;
	namePose.normalize();

	Vec3d XYname;
	if (!sPainter.getProjector()->projectCheck(XYZname, XYname))
	{
		return;
	}

	sPainter.setColor(nameColor, 1.0f);
	sPainter.drawText(static_cast<float>(XYname[0]), static_cast<float>(XYname[1]), englishName, 0.,
	                  -sPainter.getFontMetrics().boundingRect(englishName).width() / 2, 0, false);
}

void scm::ScmConstellation::drawNames(StelCore *core, StelPainter &sPainter) const
{
	if (isHidden)
	{
		return;
	}

	drawNames(core, sPainter, defaultConstellationNameColor);
}

QJsonObject scm::ScmConstellation::toJson(const QString &skyCultureId, const bool mergeLines) const
{
	QJsonObject json;

	// Assemble lines object
	QJsonArray linesArray;

	if (!isDarkConstellation)
	{
		// not a dark constellation, so we can add stars
		for (const auto &star : stars)
		{
			linesArray.append(star.toJson());
		}
	}
	else
	{
		// dark constellation, so only add coordinates
		for (const auto &coord : coordinates)
		{
			linesArray.append(coord.toJson());
		}
	}

	if (mergeLines)
	{
		mergeLinesIntoPolylines(linesArray);
	}

	json["id"]    = "CON " + skyCultureId + " " + id;
	json["lines"] = linesArray;
	if (artwork.getHasArt() && !artworkPath.isEmpty())
	{
		QFileInfo fileInfo(artworkPath);
		// the '/' separator is default in all sky cultures
		json["image"] = artwork.toJson("illustrations/" + fileInfo.fileName());
	}

	// Assemble common name object
	QJsonObject commonNameObj;
	commonNameObj["english"] = englishName;
	if (nativeName.has_value())
	{
		commonNameObj["native"] = nativeName.value();
	}
	if (pronounce.has_value())
	{
		commonNameObj["pronounce"] = pronounce.value();
	}
	if (ipa.has_value())
	{
		commonNameObj["ipa"] = ipa.value();
	}
	if (references.has_value() && !references->isEmpty())
	{
		QJsonArray refsArray;
		for (const auto &ref : references.value())
		{
			refsArray.append(ref);
		}
		commonNameObj["references"] = refsArray;
	}
	json["common_name"] = commonNameObj;

	return json;
}

bool scm::ScmConstellation::saveArtwork(const QString &directory)
{
	if (!artwork.getHasArt())
	{
		qWarning() << "SkyCultureMaker: The artwork of the constellation " << id << " has no art.";
		return true; // Not an error just a warning
	}

	QString filename = id.split(" ").back(); // Last part of id as usually used as the illustrations name
	QString filepath = directory + QDir::separator() + filename + ".png"; // Write every illustrations as png
	artworkPath      = filepath;
	return artwork.save(filepath);
}

void scm::ScmConstellation::updateTextPosition()
{
	XYZname.set(0., 0., 0.);
	for (CoordinateLine p : coordinates)
	{
		XYZname += p.end;
		XYZname += p.start;
	}
	XYZname.normalize();
}

void scm::ScmConstellation::hide()
{
	isHidden = true;
}

void scm::ScmConstellation::show()
{
	isHidden = false;
}

void scm::ScmConstellation::mergeLinesIntoPolylines(QJsonArray &lines) const
{
	if (lines.size() < 2)
	{
		// Nothing to merge
		return;
	}

	// Step 1: merge line ends with other line starts
	for (int growableLineIdx = 0; growableLineIdx < lines.size(); ++growableLineIdx)
	{
		QJsonArray growableLine = lines.at(growableLineIdx).toArray();

		// Look for a line that starts where the growableLine ends
		for (int attachableLineIdx = growableLineIdx + 1; attachableLineIdx < lines.size(); ++attachableLineIdx)
		{
			QJsonArray attachableLine = lines.at(attachableLineIdx).toArray();

			// Merge attachableLine into growableLine
			if (growableLine.last() == attachableLine.first())
			{
				// Append all points from attachableLine except the first (which is duplicate)
				attachableLine.removeFirst();
				for (QJsonValue attachableLinePoint : attachableLine)
				{
					growableLine.append(attachableLinePoint);
				}
				
				// Update the merged lines array
				lines[growableLineIdx] = growableLine;
				lines.removeAt(attachableLineIdx);

				// Recheck the merged line
				--growableLineIdx;
				// Reset j to growableLineIdx + 1 to continue merging
				attachableLineIdx = growableLineIdx + 1;
				break;
			}
		}
	}

	// Step 2: merge line starts with other line ends
	for (int growableLineIdx = 0; growableLineIdx < lines.size(); ++growableLineIdx)
	{
		QJsonArray growableLine = lines.at(growableLineIdx).toArray();

		// Look for a line that ends where the growableLine starts
		for (int attachableLineIdx = growableLineIdx + 1; attachableLineIdx < lines.size(); ++attachableLineIdx)
		{
			QJsonArray attachableLine = lines.at(attachableLineIdx).toArray();

			if (growableLine.first() == attachableLine.last())
			{
				QJsonArray newGrowableLine;
				// Prepend all points from attachableLine except the last (which is duplicate)
				attachableLine.removeLast();
				for (QJsonValue attachableLinePoint : attachableLine)
				{
					newGrowableLine.append(attachableLinePoint);
				}
				for (QJsonValue growableLinePoint : growableLine)
				{
					newGrowableLine.append(growableLinePoint);
				}
				growableLine = newGrowableLine;

				lines[growableLineIdx] = growableLine;
				lines.removeAt(attachableLineIdx);

				// Recheck the merged line
				--growableLineIdx;
				// Reset j to growableLineIdx + 1 to continue merging
				attachableLineIdx = growableLineIdx + 1;
				break;
			}
		}
	}
}
