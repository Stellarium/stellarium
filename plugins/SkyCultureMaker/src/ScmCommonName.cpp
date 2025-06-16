#include "ScmCommonName.hpp"

scm::ScmCommonName::ScmCommonName(const QString &id)
{
	ScmCommonName::id = id;
}

void scm::ScmCommonName::setEnglishName(const QString &name)
{
	englishName = name;
}

void scm::ScmCommonName::setNativeName(const QString &name)
{
	nativeName = name;
}

void scm::ScmCommonName::setPronounce(const QString &pronounce)
{
	ScmCommonName::pronounce = pronounce;
}

void scm::ScmCommonName::setIpa(const QString &ipa)
{
	ScmCommonName::ipa = ipa;
}

void scm::ScmCommonName::setReferences(const std::vector<int> &refs)
{
	references = refs;
}

QString scm::ScmCommonName::getEnglishName() const
{
	return englishName;
}

std::optional<QString> scm::ScmCommonName::getIpa() const
{
	return ipa;
}

std::optional<std::vector<int>> scm::ScmCommonName::getReferences() const
{
	return references;
}
