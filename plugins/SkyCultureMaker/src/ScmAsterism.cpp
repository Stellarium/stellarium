#include "ScmAsterism.hpp"

void scm::ScmAsterism::setCommonName(const ScmCommonName &name)
{
	commonName = name;
}

void scm::ScmAsterism::setId(const QString &id)
{
	ScmAsterism::id = id;
}

scm::ScmCommonName scm::ScmAsterism::getCommonName() const
{
	return commonName;
}

QString scm::ScmAsterism::getId() const
{
	return id;
}
