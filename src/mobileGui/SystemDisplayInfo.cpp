#include "SystemDisplayInfo.hpp"

#ifdef ANDROID
#include "../android/JavaWrapper.hpp"
#endif

QTM_USE_NAMESPACE


SystemDisplayInfo::SystemDisplayInfo(QObject *parent) :
	QSystemDisplayInfo(parent)
{
	currOrientation = QSystemDisplayInfo::orientation(0);

	//Calculate screen density
	int dpi;

#ifdef ANDROID
	//if using Android, get the abstract density scale via JNI
	dpi = JavaWrapper::getDensityDpi();
#else
	//if not, figure it out the straight-forward way
	dpi = (getDPIWidth() + getDPIHeight()) / 2;
#endif

	if(dpi < MEDIUM_DPI)
	{
		m_density = LOW_DPI_DENSITY;
		m_dpiBucket = LOW_DPI;
	}
	else if(dpi < HIGH_DPI)
	{
		m_density = MEDIUM_DPI_DENSITY;
		m_dpiBucket = MEDIUM_DPI;
	}
	else if(dpi < XHIGH_DPI)
	{
		m_density = HIGH_DPI_DENSITY;
		m_dpiBucket = HIGH_DPI;
	}
	else
	{
		m_density = XHIGH_DPI_DENSITY;
		m_dpiBucket = XHIGH_DPI;
	}


	densityChanged();
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

SystemDisplayInfo::DpiBucket SystemDisplayInfo::dpiBucket()
{
	return m_dpiBucket;
}

float SystemDisplayInfo::density()
{
	return m_density;
}

int SystemDisplayInfo::dpToPixels(int dp)
{
	return static_cast<int>(dp * m_density);
}
