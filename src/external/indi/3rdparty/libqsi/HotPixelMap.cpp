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
#include "HotPixelMap.h"
#include "QSI_Registry.h"
#include <iostream>
#include <string>
#include <sstream>

#define REGMAPROOT _T("SOFTWARE/QSI/Map/")

HotPixelMap::HotPixelMap(void)
{
	m_bEnable = false;
}

HotPixelMap::HotPixelMap(std::string Serial)
{
	int lResult = 0;
	int dwX, dwY;
	int dSize = 4;
	int RemapCount = 0;
	QSI_Registry reg;

	this->serial = Serial;
	std::string Root = std::string(REGMAPROOT);
	Root += Serial;
	Root += _T("/");

	int dwEnable = 0;
	lResult = reg.RegQueryValueEx(Root, _T("Enable"), 0, 0, &dwEnable, dSize);
	m_bEnable = (lResult == 0 && dwEnable != 0) ? true:false;

	if ( m_bEnable )
	{
		dSize = 4;
		std::string XName = _T("X0");
		std::string YName = _T("Y0");
		RemapCount = 0;
		while (	(lResult = reg.RegQueryValueEx(Root, XName, 0, 0, &dwX, dSize) == 0) &&
				(lResult = reg.RegQueryValueEx(Root, YName, 0, 0, &dwY, dSize) == 0) )
		{
			std::stringstream cnv;
			HotMap.push_back(Pixel(dwX, dwY));
			RemapCount++;
			cnv << RemapCount;
			XName = std::string(_T("X")); 
			XName += cnv.str();
			YName = std::string(_T("Y"));
			YName += cnv.str();
		}
	}
}

HotPixelMap::~HotPixelMap(void)
{
} 

bool HotPixelMap::Save(void)
{
	int RemapCount = 0;
	std::string XName = _T("X0");
	std::string YName = _T("Y0");
	std::string Root = std::string(REGMAPROOT);
	std::vector<Pixel>::iterator vi;
	QSI_Registry reg;

	if (serial == _T("")) serial = _T("Default");
	Root += serial;
	Root += _T("/");

	// First delete old key data (and subKeys)
	reg.RegDelnode (Root);

	reg.SetNumber(Root, std::string(_T("Enable")), m_bEnable?1:0);
	for (vi = HotMap.begin(); vi != HotMap.end(); vi++)
	{
		std::stringstream cnv;
		cnv << RemapCount;
		XName = std::string(_T("X")); 
		XName += cnv.str();
		YName = std::string(_T("Y"));
		YName += cnv.str();
		RemapCount++;

		reg.SetNumber(Root, XName, (*vi).x);
		reg.SetNumber(Root, YName, (*vi).y);
	}
	return true;
}

void HotPixelMap::Remap(	BYTE * Image, int RowPad, QSI_ExposureSettings Exposure,
							QSI_DeviceDetails Details, USHORT ZeroPixel, QSILog * log)
{
	int pIndex;
	std::vector<Pixel>::iterator vi;

	if (!m_bEnable)
		return;
	log->Write(2, _T("Hot Pixel Remap enabled."));

	for (vi = HotMap.begin(); vi != HotMap.end(); vi++)
	{
		log->Write(2, _T("Remap pixel: x=%d, y=%d"), (*vi).x, (*vi).y);

		if (FindTargetPixelIndex(*vi, RowPad, Exposure, Details, log, &pIndex))
		{
			log->Write(2, _T("Remap pixel: x=%d, y=%d, old value: %d, new value: %d."),
							(*vi).x, (*vi).y, *(USHORT*)(&Image[pIndex]), ZeroPixel); 
			*(USHORT*)(&Image[pIndex]) = ZeroPixel;
		}
	}
}

bool HotPixelMap::FindTargetPixelIndex(	Pixel pxIn, int RowPad, QSI_ExposureSettings Exposure,
										QSI_DeviceDetails Details, QSILog * log, int * pIndex)
{
	int iStartX;
	int iStartY;
	int iSizeX;
	int iSizeY;
	int iBinnedLocX;
	int iBinnedLocY;
	int iRowLen;
	static const int BYTESPERPIXEL = 2;

	// Is the requested remap pixel in the array range of the camera?
	if (pxIn.x >= Details.ArrayColumns || pxIn.y >= Details.ArrayRows)
	{
		log->Write(2, _T("Remap pixel: x=%d, y=%d not in CCD imager area."), pxIn.x, pxIn.y);
		return false;
	}

	// Un-Bin the parameters of the image and check if this pixel is in the requested frame
	iStartX = Exposure.ColumnOffset * Exposure.BinFactorX;
	iStartY = Exposure.RowOffset * Exposure.BinFactorY;
	iSizeX =  Exposure.ColumnsToRead * Exposure.BinFactorX;
	iSizeY =  Exposure.RowsToRead * Exposure.BinFactorY;
	// RowLen and RowPad in bytes (for CCDSoft padding)
	// The "Row length in bytes" is Exposure.ColumnsToRead number of columns times two
	iRowLen =  Exposure.ColumnsToRead * BYTESPERPIXEL; 

	// Is the remapped pixel in the target area?
	if (pxIn.x >= iStartX && pxIn.x < iStartX + iSizeX &&
		pxIn.y >= iStartY && pxIn.y < iStartY + iSizeY )
	{
		// Hit on remap
		// Now cal the binned pixel index of the hot pixel
		iBinnedLocX = (pxIn.x / Exposure.BinFactorX) - Exposure.ColumnOffset;
		iBinnedLocY = (pxIn.y / Exposure.BinFactorY) - Exposure.RowOffset;
		// Calc image array index in bytes, caller will use that to replace pixel
		*pIndex = (iBinnedLocX * BYTESPERPIXEL) + ((iRowLen + RowPad) * iBinnedLocY);
		log->Write(2, _T("Remap pixel: x=%d, y=%d at image index: %d"), pxIn.x, pxIn.y, *pIndex);
		return true;
	}
	else
	{
		log->Write(2, _T("Remap pixel: x=%d, y=%d not in image area."), pxIn.x, pxIn.y);
		return false;
	}
}

std::vector<Pixel> HotPixelMap::GetPixels(void)
{
	return this->HotMap;
}

void HotPixelMap::SetPixels(std::vector<Pixel> map)
{
	this->HotMap = map;
}
