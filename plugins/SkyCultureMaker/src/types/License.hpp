#ifndef SCM_LICENSE_HPP
#define SCM_LICENSE_HPP

#include <map>
#include <vector>
#include <QString>

namespace scm
{

struct License
{
	QString name;
	QString description;

	License(const QString& name, const QString& description)
		: name(name)
		, description(description)
	{
	}
};

// Enum class to represent different types of licenses
enum class LicenseType
{
	NONE,
	CC0,
	CC_BY
};

const std::map<LicenseType, License> LICENSES = {
	{LicenseType::NONE, License("None", "Please select a valid license.")},
	{LicenseType::CC0, License("CC0",
        "This file is made available under the Creative Commons CC0 1.0 Universal Public Domain Dedication. "
        "The person who associated a work with this deed has dedicated the work to the public domain by "
        "waiving all of his or her rights to the work worldwide under copyright law, including all related "
        "and neighboring rights, to the extent allowed by law. You can copy, modify, distribute and perform "
        "the work, even for commercial purposes, all without asking permission.")},
	{LicenseType::CC_BY, License("CC BY", 
        "This work is licensed under the Creative Commons Attribution 4.0 License; Reusage allowed" 
        " - please mention author(s).")}
};

} // namespace scm



#endif // SCM_LICENSE_HPP
