#ifndef SCM_DESCRIPTION_HPP
#define SCM_DESCRIPTION_HPP

#include "Classification.hpp"
#include <map>
#include <vector>
#include <QMetaType>
#include <QString>

namespace scm
{

/**
 * @brief The Description struct represents a sky culture description.
 * It contains various fields that describe the sky culture, including its name,
 * geographical region, sky features, celestial objects, and more.
 * It also includes a classification type to categorize the sky culture.
 * The `isComplete` method checks if all required fields are filled.
 */
struct Description
{
	QString name;
	QString geoRegion;
	QString sky;
	QString moonAndSun;
	QString zodiac;
	QString planets;
	QString constellations;
	QString milkyWay;
	QString otherObjects;
	QString about;
	QString authors;
	QString acknowledgements;
	QString references;
	scm::ClassificationType classification;

	bool isComplete() const
	{
		return !name.trimmed().isEmpty() && !geoRegion.trimmed().isEmpty() && !sky.trimmed().isEmpty() &&
		       !moonAndSun.trimmed().isEmpty() && !zodiac.trimmed().isEmpty() && !planets.trimmed().isEmpty() &&
		       !constellations.trimmed().isEmpty() && !milkyWay.trimmed().isEmpty() &&
		       !otherObjects.trimmed().isEmpty() && !about.trimmed().isEmpty() &&
		       !authors.trimmed().isEmpty() && !acknowledgements.trimmed().isEmpty() &&
		       !references.trimmed().isEmpty() && classification != scm::ClassificationType::NONE;
	}
};
} // namespace scm

Q_DECLARE_METATYPE(scm::Description);

#endif // SCM_DESCRIPTION_HPP
