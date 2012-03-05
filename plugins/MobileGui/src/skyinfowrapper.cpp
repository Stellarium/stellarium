#include "skyinfowrapper.hpp"

#include "../../../src/core/StelModuleMgr.hpp"
#include "../../../src/core/StelApp.hpp"
#include "../../../src/core/StelObjectMgr.hpp"

SkyInfoWrapper::SkyInfoWrapper(QObject *parent) :
    QObject(parent)
{
	infoTextFilters = static_cast<StelObject::InfoStringGroup>(StelObject::Magnitude|StelObject::RaDecJ2000|StelObject::AltAzi|StelObject::Distance|StelObject::Size|StelObject::Extra1|StelObject::Extra2|StelObject::Extra3);
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
