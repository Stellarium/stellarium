#include "ScmSkyCulture.hpp"

void scm::ScmSkyCulture::setId(QString id)
{
	ScmSkyCulture::id = id;
}

void scm::ScmSkyCulture::setRegion(QString region)
{
	ScmSkyCulture::region = region;
}

void scm::ScmSkyCulture::setClassificationType(ClassificationType classificationType)
{
	ScmSkyCulture::classificationType = classificationType;
}

void scm::ScmSkyCulture::setFallbackToInternationalNames(bool fallback)
{
	ScmSkyCulture::fallbackToInternationalNames = fallback;
}

void scm::ScmSkyCulture::addAsterism(scm::ScmAsterism asterism)
{
	asterisms.push_back(asterism);
}

void scm::ScmSkyCulture::removeAsterism(QString id)
{
	asterisms.erase(
	    remove_if(begin(asterisms), end(asterisms), [id](scm::ScmAsterism const &a) { return a.getId() == id; }),
	    end(asterisms));
}

void scm::ScmSkyCulture::addConstellation(QString id,
					  std::vector<CoordinateLine> coordinates,
					  std::vector<StarLine> stars)
{
	scm::ScmConstellation constellationObj(coordinates, stars);
	constellationObj.setId(id);
	constellations.push_back(constellationObj);
}

void scm::ScmSkyCulture::removeConstellation(QString id)
{
	constellations.erase(remove_if(begin(constellations),
				       end(constellations),
				       [id](ScmConstellation const &c) { return c.getId() == id; }),
			     end(constellations));
}

scm::ScmConstellation *scm::ScmSkyCulture::getConstellation(QString id)
{
	for (auto &constellation : constellations)
	{
		if (constellation.getId() == id)
			return &constellation;
	}
	return nullptr;
}

std::vector<scm::ScmConstellation> *scm::ScmSkyCulture::getConstellations()
{
	return &constellations;
}

void scm::ScmSkyCulture::setLicense(scm::LicenseType license)
{
	ScmSkyCulture::license = license;
}

scm::LicenseType scm::ScmSkyCulture::getLicense() const
{
	return ScmSkyCulture::license;
}

void scm::ScmSkyCulture::setAuthors(const QString authors)
{
	ScmSkyCulture::authors = authors;
}

QString scm::ScmSkyCulture::getAuthors() const
{
	return ScmSkyCulture::authors;
}

void scm::ScmSkyCulture::draw(StelCore *core)
{
	for (auto &constellation : constellations)
	{
		constellation.drawConstellation(core);
	}
}