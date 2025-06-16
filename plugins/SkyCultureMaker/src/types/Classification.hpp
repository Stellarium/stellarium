#ifndef SCM_CLASSIFICATION_HPP
#define SCM_CLASSIFICATION_HPP

#include <map>
#include <vector>
#include <QMetaType>
#include <QString>

namespace scm
{

/**
 * @brief The Classification struct represents a classification type for sky cultures.
 * All information was taken from the Stellarium guide.
 */
struct Classification
{
	QString name;
	QString description;

	Classification(const QString& name, const QString& description)
		: name(name)
		, description(description)
	{
	}
};

// Enum class to represent different types of classifications
enum class ClassificationType
{
	NONE = 0,
	INCOMPLETE,
	PERSONAL,
	TRADITIONAL,
	ETHNOGRAPHIC,
	HISTORICAL,
	SINGLE,
	COMPARATIVE
};

inline QString classificationTypeToString(ClassificationType type)
{
	switch (type)
	{
	case ClassificationType::NONE: return "None";
	case ClassificationType::INCOMPLETE: return "Incomplete";
	case ClassificationType::PERSONAL: return "Personal";
	case ClassificationType::TRADITIONAL: return "Traditional";
	case ClassificationType::ETHNOGRAPHIC: return "Ethnographic";
	case ClassificationType::HISTORICAL: return "Historical";
	case ClassificationType::SINGLE: return "Single";
	case ClassificationType::COMPARATIVE: return "Comparative";
	default: qDebug() << "Unknown ClassificationType: " << static_cast<int>(type); return "Unkown";
	}
}

const std::map<ClassificationType, Classification> CLASSIFICATIONS = {
	{ClassificationType::NONE, Classification("None", "Please select a valid classification.")},
	{
         ClassificationType::INCOMPLETE,
         Classification("Incomplete", "This is a personally developed sky culture which is not founded in "
                                             "published historical or ethnological research. Stellarium may include it "
                                             "when it is “pretty enough” without really approving its contents."),
	 },
	{
         ClassificationType::PERSONAL,
         Classification("Personal",
         "This is a privately developed sky culture, not based on published ethnographic or "
                               "historical research, and not supported by a noteworthy community. It may be included "
                               "in Stellarium when it is “pretty enough” without really approving its contents."),
	 },
	{
         ClassificationType::TRADITIONAL,
         Classification("Traditional",
         "The content represents “common” knowledge by several members of an ethnic community, "
                               "and the sky culture has been developed by members of such community. Our “Modern” sky "
                               "culture is a key example: rooted in antiquity it has evolved for about 2500 years in "
                               "what is now commonly known as “western” world, and modern astronomers use it."),
	 },
	{
         ClassificationType::ETHNOGRAPHIC,
         Classification("Ethnographic", "The data of the sky culture is provided by ethnographic researchers "
                                               "based on interviews of indigenous people."),
	 },
	{
         ClassificationType::HISTORICAL,
         Classification("Historical", "The sky culture is based on historical written sources from a (usually "
                                             "short) period of the past."),
	 },
	{
         ClassificationType::SINGLE,
         Classification("Single", "The data of the sky culture is based on a single source like a historical "
                                         "atlas, or related publications of a single author."),
	 },
	{
         ClassificationType::COMPARATIVE,
         Classification("Comparative",
         "This sky culture is a special-purpose composition of e.g. artwork from one and stick "
                               "figures from another sky culture, and optionally asterisms as representations of a "
                               "third. Or comparison of two stick figure sets in constellations and asterisms. These "
                               "figures sometimes will appear not to fit together well. This may be intended, to "
                               "explain and highlight just those differences! The description text must clearly "
                               "explain and identify all sources and how these differences should be interpreted."),
	 },
};

} // namespace scm

Q_DECLARE_METATYPE(scm::ClassificationType);

#endif // SCM_CLASSIFICATION_HPP
