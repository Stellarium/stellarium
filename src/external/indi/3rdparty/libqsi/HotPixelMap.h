/*****************************************************************************************
NAME
 HotPixelMap

DESCRIPTION
 Class to specify info to define a HotPIxelMap

COPYRIGHT (C)
 QSI (Quantum Scientific Imaging) 2011

REVISION HISTORY
DRC 03.23.11 Original Version
******************************************************************************************/
#ifndef HOTPIXELMAP_H
#define HOTPIXELMAP_H

/**
	@author David Challis
*/
#include "QSI_Global.h"
#include "QSILog.h"
#include "qsiapi.h"
#include <vector>
#include <string>

class HotPixelMap
{
public:
	HotPixelMap(void);
	HotPixelMap(std::string Serial);
	~HotPixelMap(void);
	void Remap(	BYTE * Image, int RowPad, QSI_ExposureSettings Exposure,
				QSI_DeviceDetails Details, USHORT ZeroPixel, QSILog * log);
	bool Save(void);
	std::vector<Pixel> GetPixels(void);
	void SetPixels(std::vector<Pixel> map);
	bool m_bEnable;
private:
	bool FindTargetPixelIndex(	Pixel pxIn, int RowPad, QSI_ExposureSettings Exposure, QSI_DeviceDetails Details, QSILog * log, int * pIndex);
	std::vector<Pixel> HotMap;
	std::string serial;
};

#endif
