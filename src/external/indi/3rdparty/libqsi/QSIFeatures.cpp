/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * qsilib
 */

#include "QSIFeatures.h"

QSIFeatures::QSIFeatures(void)
{
	m_iFeaturesReturned = 0;
	for (int i = 0; i < IMAXFEATURES; i++)
	{
		m_Features[i] = 0;
	}
}

QSIFeatures::~QSIFeatures(void)
{
}

void QSIFeatures::GetFeatures( BYTE * pData, int FeaturesInArray )
{
	for (int i = 0; i < IMAXFEATURES; i++)
	{
		if (i < FeaturesInArray)
			m_Features[i] = pData[i];
		else
			m_Features[i] = 0;
	}
}

bool QSIFeatures::HasHSRExposure()
{
	return (m_Features[HSREXPOSURE] == 1)?true:false;
}

bool QSIFeatures::HasPVIMode()
{
	return (m_Features[PVIMODE] == 1)?true:false;
}

bool QSIFeatures::HasLockCamera()
{
	return (m_Features[LOCKCAMERA] == 1)?true:false;
}

bool QSIFeatures::HasBasicHWTrigger()
{
	return (m_Features[BASICHWTRIGGER] == 1)?true:false;
}

