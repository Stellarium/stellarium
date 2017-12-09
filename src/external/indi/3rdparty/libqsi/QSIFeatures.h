/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * qsilib
 */

#ifndef _QSI_FEATURES_H_
#define _QSI_FEATURES_H_
#include "WinTypes.h"
class QSIFeatures
{
public:
	QSIFeatures(void);
	~QSIFeatures(void);

	static const int IMAXFEATURES = 254;
	void GetFeatures( BYTE * pData, int FeaturesInArray );

	bool HasHSRExposure();
	bool HasPVIMode();
	bool HasLockCamera();
	bool HasBasicHWTrigger();

private:
	// Special features array;
	static const int HSREXPOSURE		= 0;  // AKA GetFocusFrame
	static const int PVIMODE			= 1;
	static const int PVIMODEONEFLAGS	= 2;
	static const int LOCKCAMERA			= 3;
	static const int BASICHWTRIGGER		= 4;
	BYTE m_Features[IMAXFEATURES];
	int m_iFeaturesReturned;
};

#endif // _QSI_FEATURES_H_
