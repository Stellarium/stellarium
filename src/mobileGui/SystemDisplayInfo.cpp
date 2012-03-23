#include "SystemDisplayInfo.hpp"

QTM_USE_NAMESPACE

SystemDisplayInfo::SystemDisplayInfo(QObject *parent) :
	QSystemDisplayInfo(parent)
{
	currOrientation = QSystemDisplayInfo::orientation(0);
}

int SystemDisplayInfo::displayBrightness()
{
	return QSystemDisplayInfo::displayBrightness(0);
}

int SystemDisplayInfo::colorDepth()
{
	return QSystemDisplayInfo::colorDepth(0);
}

QSystemDisplayInfo::DisplayOrientation SystemDisplayInfo::orientation()
{
	return currOrientation;
}

float SystemDisplayInfo::contrast()
{
	return QSystemDisplayInfo::contrast(0);
}

int SystemDisplayInfo::getDPIWidth()
{
	return QSystemDisplayInfo::getDPIWidth(0);
}

int SystemDisplayInfo::getDPIHeight()
{
	return QSystemDisplayInfo::getDPIHeight(0);
}

int SystemDisplayInfo::physicalHeight()
{
	return QSystemDisplayInfo::physicalHeight(0);
}

int SystemDisplayInfo::physicalWidth()
{
	return QSystemDisplayInfo::physicalWidth(0);
}

QSystemDisplayInfo::BacklightState SystemDisplayInfo::backlightStatus()
{
	return QSystemDisplayInfo::backlightStatus(0);
}

void SystemDisplayInfo::changeOrientation(QSystemDisplayInfo::DisplayOrientation orientation)
{
	this->currOrientation = orientation;
}
