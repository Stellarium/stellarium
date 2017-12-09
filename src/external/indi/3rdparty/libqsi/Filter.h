/*****************************************************************************************
NAME
 Filter

DESCRIPTION
 Class to specify info to define a filter

COPYRIGHT (C)
 QSI (Quantum Scientific Imaging) 2011

REVISION HISTORY
DRC 03.23.11 Original Version
******************************************************************************************/
#pragma once
#include <string>
#include <string.h>
#include <stdlib.h>

class Filter
{
public:
	Filter(void);
	~Filter(void);
	std::string Name;
	int32_t Offset = 0;
	short Trim = 0;
};
