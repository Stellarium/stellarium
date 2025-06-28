#include "ScmConstellation.hpp"
#include <QDir>
#include <QFileInfo>

scm::ScmConstellation::ScmConstellation(const std::vector<scm::CoordinateLine> &coordinates,
                                        const std::vector<scm::StarLine> &stars)
	: constellationCoordinates(coordinates)
	, constellationStars(stars)
{
	QSettings *conf = StelApp::getInstance().getSettings();
	constellationNameFont.setPixelSize(conf->value("viewing/constellation_font_size", 15).toInt());

	QString defaultColor          = conf->value("color/default_color", "0.5,0.5,0.7").toString();
	defaultConstellationLineColor = Vec3f(conf->value("color/const_lines_color", defaultColor).toString());
	defaultConstellationNameColor = Vec3f(conf->value("color/const_names_color", defaultColor).toString());

	updateTextPosition();
}

void scm::ScmConstellation::setId(const QString &id)
{
	ScmConstellation::id = id;
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

void scm::ScmConstellation::setPronounce(const std::optional<QString> &pronounce)
{
	ScmConstellation::pronounce = pronounce;
}

void scm::ScmConstellation::setIPA(const std::optional<QString> &ipa)
{
	ScmConstellation::ipa = ipa;
}

void scm::ScmConstellation::setArtwork(const ScmConstellationArtwork &artwork)
{
	ScmConstellation::artwork = artwork;
}

void scm::ScmConstellation::setConstellation(const std::vector<CoordinateLine> &coordinates,
                                             const std::vector<StarLine> &stars)
{
	constellationCoordinates = coordinates;
	constellationStars       = stars;

	updateTextPosition();
}

void scm::ScmConstellation::drawConstellation(StelCore *core, const Vec3f &lineColor, const Vec3f &nameColor) const
{
	StelPainter painter(core->getProjection(drawFrame));
	painter.setBlending(true);
	painter.setLineSmooth(true);
	painter.setFont(constellationNameFont);

	bool alpha = 1.0f;
	painter.setColor(lineColor, alpha);

	for (CoordinateLine p : constellationCoordinates)
	{
		painter.drawGreatCircleArc(p.start, p.end);
	}

	drawNames(core, painter, nameColor);

	artwork.draw(core, painter);
}

void scm::ScmConstellation::drawConstellation(StelCore *core) const
{
	drawConstellation(core, defaultConstellationLineColor, defaultConstellationNameColor);
}

void scm::ScmConstellation::drawNames(StelCore *core, StelPainter &sPainter, const Vec3f &nameColor) const
{
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
	drawNames(core, sPainter, defaultConstellationNameColor);
}

QJsonObject scm::ScmConstellation::toJson(const QString &skyCultureId) const
{
	QJsonObject json;

	// Assemble lines object
	QJsonArray linesArray;

	if (constellationStars.size() != 0)
	{
		// Stars are NOT empty
		for (const auto &star : constellationStars)
		{
			linesArray.append(star.toJson());
		}
	}
	else
	{
		// Stars are empty, use the coordinates
		for (const auto &coord : constellationCoordinates)
		{
			linesArray.append(coord.toJson());
		}
	}

	json["id"]    = "CON " + skyCultureId + " " + id;
	json["lines"] = linesArray;
	if (artwork.getHasArt() == true && !artworkPath.isEmpty())
	{
		QFileInfo fileInfo(artworkPath);
		// the '/' separator is default in all skycultures
		json["image"] = artwork.toJson("illustrations/" + fileInfo.fileName());
	}
	else
	{
		qWarning() << "WARNING: The artwork of this constellation " << id
			   << " has no art or was not saved to a file.";
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
		qWarning() << "WARNING: The artwork of this constellation " << id << " has no art.";
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
	for (CoordinateLine p : constellationCoordinates)
	{
		XYZname += p.end;
		XYZname += p.start;
	}
	XYZname.normalize();
}
