#include "StelWrapper.hpp"

#include "StelModuleMgr.hpp"
#include "StelApp.hpp"
#include "StelObjectMgr.hpp"
#include "StelMovementMgr.hpp"
#include "MobileGui.hpp"

StelWrapper::StelWrapper(QObject *parent) :
    QObject(parent)
{
	infoTextFilters = StelObject::AllInfo;
}

//! Get a pointer on the info panel used to display selected object info
void StelWrapper::setInfoTextFilters(const StelObject::InfoStringGroup& aflags)
{
	infoTextFilters = aflags;
}

const StelObject::InfoStringGroup& StelWrapper::getInfoTextFilters() const
{
	return infoTextFilters;
}

void StelWrapper::toggleAllInfo()
{
	infoTextFilters = infoTextFilters == StelObject::InfoStringGroup(StelObject::AllInfo) ? StelObject::InfoStringGroup(StelObject::ShortInfo) : StelObject::InfoStringGroup(StelObject::AllInfo);
}

QString StelWrapper::getInfoText()
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

double StelWrapper::getCurrentFov()
{
	return GETSTELMODULE(StelMovementMgr)->getCurrentFov();
}

void StelWrapper::setFov(qreal f)
{
	GETSTELMODULE(StelMovementMgr)->setFov(static_cast<double>(f));
}
