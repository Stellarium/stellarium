/*****************************************************************************************
NAME
 FilterWheel

DESCRIPTION
 Class to specify info to define a filter Wheel

COPYRIGHT (C)
 QSI (Quantum Scientific Imaging) 2011

REVISION HISTORY
DRC 03.23.11 Original Version
******************************************************************************************/
#pragma once
#include <string>
#include <vector>
#include "Filter.h"

class FilterWheel
{

public:
	FilterWheel();
	FilterWheel(int iNumFilters);
	~FilterWheel();

	static void GetWheels( std::string strSerialNumber, std::vector<FilterWheel>  * Wheels, int iNumFiltersExpected  );
	bool LoadFromRegistry( std::string strSerialNumber, std::string strName, int iNumFiltersExpected );
	void SaveToRegistry( std::string  strSerialNumber );
	void DeleteFromRegistry( std::string  strSerialNumber );
	void AddFilter(Filter filter);
	std::vector<Filter> Filters;
	std::string Name;
	int m_iNumFilters = 0;
};
