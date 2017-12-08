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
#include "Filter.h"

Filter::Filter(void)
{
	Name = "Unassigned";
	Offset = 0;
	Trim = 0;
}

Filter::~Filter(void)
{
}
