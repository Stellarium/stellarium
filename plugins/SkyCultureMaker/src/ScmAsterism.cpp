#include "ScmAsterism.hpp"

void scm::ScmAsterism::setCommonName(ScmCommonName name)
{
    commonName = name;
}

void scm::ScmAsterism::setId(QString id)
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
