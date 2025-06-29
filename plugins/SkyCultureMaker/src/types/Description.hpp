#ifndef SCM_DESCRIPTION_HPP
#define SCM_DESCRIPTION_HPP

#include "Classification.hpp"
#include "License.hpp"
#include <map>
#include <vector>
#include <QMetaType>
#include <QString>

namespace scm
{

/**
 * @brief The Description struct represents a sky culture description.
 */
struct Description
{
	QString name;
	QString authors;
	scm::LicenseType license;
	QString cultureDescription;
	QString about;

	QString geoRegion;
	QString sky;
	QString moonAndSun;
	QString planets;
	QString zodiac;
	QString milkyWay;
	QString otherObjects;

	QString constellations;
	QString references;
	QString acknowledgements;
	scm::ClassificationType classification;

	/**
	 * @brief Check if the description is complete.
	 * @return true if all required fields are filled, false otherwise.
	 */
	bool isComplete() const
	{
		return !name.trimmed().isEmpty() && !geoRegion.trimmed().isEmpty() && !sky.trimmed().isEmpty() &&
		       !moonAndSun.trimmed().isEmpty() && !zodiac.trimmed().isEmpty() && !planets.trimmed().isEmpty() &&
		       !constellations.trimmed().isEmpty() && !milkyWay.trimmed().isEmpty() &&
		       !otherObjects.trimmed().isEmpty() && !about.trimmed().isEmpty() &&
		       !authors.trimmed().isEmpty() && !acknowledgements.trimmed().isEmpty() &&
		       !references.trimmed().isEmpty() && classification != scm::ClassificationType::NONE &&
				license != scm::LicenseType::NONE;
	}
};
} // namespace scm

Q_DECLARE_METATYPE(scm::Description);

#endif // SCM_DESCRIPTION_HPP
