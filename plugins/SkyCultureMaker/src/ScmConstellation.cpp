#include "ScmConstellation.hpp"

scm::ScmConstellation::ScmConstellation(std::vector<scm::CoordinateLine> coordinates, std::vector<scm::StarLine> stars)
	: constellationCoordinates(coordinates)
	, constellationStars(stars)
{
	QSettings* conf = StelApp::getInstance().getSettings();
	constellationLabelFont.setPixelSize(conf->value("viewing/constellation_font_size", 15).toInt());
 
	QString defaultColor = conf->value("color/default_color", "0.5,0.5,0.7").toString();
	colorDrawDefault = Vec3f(conf->value("color/const_lines_color", defaultColor).toString());
	colorLabelDefault = Vec3f(conf->value("color/const_names_color", defaultColor).toString());
}

void scm::ScmConstellation::setId(QString id)
{
	ScmConstellation::id = id;
}

QString scm::ScmConstellation::getId() const
{
	return id;
}

void scm::ScmConstellation::setEnglishName(QString name)
{
	englishName = name;
}

QString scm::ScmConstellation::getEnglishName() const
{
	return englishName;
}

void scm::ScmConstellation::setNativeName(std::optional<QString> name)
{
	nativeName = name;
}

void scm::ScmConstellation::setPronounce(std::optional<QString> pronounce)
{
	ScmConstellation::pronounce = pronounce;
}

void scm::ScmConstellation::setIPA(std::optional<QString> ipa)
{
	ScmConstellation::ipa = ipa;
}

void scm::ScmConstellation::setConstellation(std::vector<CoordinateLine> coordinates, std::vector<StarLine> stars)
{
	constellationCoordinates = coordinates;
	constellationStars = stars;
}

void scm::ScmConstellation::drawConstellation(StelCore *core, Vec3f color)
{
	StelPainter painter(core->getProjection(drawFrame));
	painter.setBlending(true);
	painter.setLineSmooth(true);
	painter.setFont(constellationLabelFont);
	
	bool alpha = 1.0f;
	painter.setColor(color, alpha);

	XYZname.set(0.,0.,0.);
	for (CoordinateLine p : constellationCoordinates)
	{
		painter.drawGreatCircleArc(p.start, p.end);
		XYZname += p.end;
		XYZname += p.start;
	}

	XYZname.normalize();

	drawNames(core, painter, colorLabelDefault);
}

void scm::ScmConstellation::drawConstellation(StelCore *core)
{
	drawConstellation(core, colorDrawDefault);
}

void scm::ScmConstellation::drawNames(StelCore *core, StelPainter sPainter, Vec3f labelColor) 
{
	sPainter.setBlending(true);

	Vec3d velocityObserver(0.);
	if (core->getUseAberration())
	{
		velocityObserver = core->getAberrationVec(core->getJDE());
	}

	XYZname+=velocityObserver;
	XYZname.normalize();

	if(!sPainter.getProjector()->projectCheck(XYZname, this->XYname))
	{
		return;
	}

	sPainter.getProjector()->project(XYZname, XYname);
	sPainter.setColor(labelColor, 1.0f);
	sPainter.drawText(static_cast<float>(XYname[0]), static_cast<float>(XYname[1]), englishName, 0., -sPainter.getFontMetrics().boundingRect(englishName).width()/2, 0, false);
}

void scm::ScmConstellation::drawNames(StelCore *core, StelPainter sPainter)
{
	drawNames(core, sPainter, colorLabelDefault);
}