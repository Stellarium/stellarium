#include "ScmConstellation.hpp"

const Vec3f scm::ScmConstellation::colorDrawDefault(0.3f, 1.f, 0.f);
const Vec3f scm::ScmConstellation::colorLabelDefault(0.3f, 1.f, 0.f);

scm::ScmConstellation::ScmConstellation(std::vector<scm::CoordinateLine> coordinates, std::vector<scm::StarLine> stars)
	: constellationCoordinates(coordinates)
	, constellationStars(stars)
{
	QSettings* conf = StelApp::getInstance().getSettings();
	constellationLabelFont.setPixelSize(conf->value("viewing/constellation_font_size", 15).toInt());
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