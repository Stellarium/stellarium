#include "skyinfowrapper.hpp"

#include "../../../src/core/StelModuleMgr.hpp"
#include "../../../src/core/StelApp.hpp"
#include "../../../src/core/StelObjectMgr.hpp"

SkyInfoWrapper::SkyInfoWrapper(QObject *parent) :
    QObject(parent)
{
	infoTextFilters = StelObject::AllInfo;
}

//! Get a pointer on the info panel used to display selected object info
void SkyInfoWrapper::setInfoTextFilters(const StelObject::InfoStringGroup& aflags)
{
	infoTextFilters = aflags;
}

const StelObject::InfoStringGroup& SkyInfoWrapper::getInfoTextFilters() const
{
	return infoTextFilters;
}

void SkyInfoWrapper::toggleAllInfo()
{
	infoTextFilters = infoTextFilters == StelObject::AllInfo ? StelObject::ShortInfo : StelObject::AllInfo;
}
QString SkyInfoWrapper::getInfoText()
{
	QList<StelObjectP> selected = GETSTELMODULE(StelObjectMgr)->getSelectedObject();
	if(selected.isEmpty())
	{
		return "";
	}
	else
	{
		return selected[0]->getInfoString(StelApp::getInstance().getCore(), static_cast<StelObject::InfoStringGroup>(infoTextFilters));
	}
}
