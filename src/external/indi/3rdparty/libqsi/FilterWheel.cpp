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
#include "FilterWheel.h"
#include "QSI_Registry.h"
#include <sstream>
#include "wincompat.h"

FilterWheel::FilterWheel()
{
	m_iNumFilters = 0;
}

FilterWheel::FilterWheel(int iNumFilters)
{
	Name = "Default";
	for (int i = 0; i < iNumFilters; i++)
	{
		Filter newFilter;
		std::stringstream ss;
		ss << i + 1;
		newFilter.Name = "Position " +  ss.str();
		newFilter.Offset = 0;
		newFilter.Trim = 0;
		Filters.push_back(newFilter);
	}
	m_iNumFilters = iNumFilters;
}

FilterWheel::~FilterWheel()
{

}

void FilterWheel::GetWheels( std::string strSerialNumber, std::vector<FilterWheel> * Wheels, int iNumFiltersExpected )
{
	QSI_Registry reg;
	std::vector<std::string> vKeys;
	std::string strKeyPath = KEY_QSI + strSerialNumber + "/FilterWheel/Names";

	reg.GetAllKeys(strKeyPath, &vKeys);

	for (int i=0; i < (int)vKeys.size(); i++)
	{
		FilterWheel fwWheel;
		if (fwWheel.LoadFromRegistry(strSerialNumber, vKeys[i], iNumFiltersExpected) )
			Wheels->push_back(fwWheel);
	}

	return;
}

bool FilterWheel::LoadFromRegistry(std::string strSerialNumber, std::string strWheelName, int iNumFiltersExpected )
{
	QSI_Registry reg;

	std::string strValue;
	std::string strKeyPath = KEY_QSI + strSerialNumber;
	strKeyPath = strKeyPath + "/FilterWheels/";
	strKeyPath = strKeyPath + strWheelName;

	Name = strWheelName;
	Filters.clear();
	m_iNumFilters = 0;

	// open registry key

	m_iNumFilters = reg.GetNumber(strKeyPath, SUBKEY_NumFilters, 0);

	if (m_iNumFilters == 0)
		return false;

	for (int i = 0; i < m_iNumFilters; i++)
	{
		Filter filter;

		std::string  strFilterNum;
		//strFilterNum.Format("%d", i+1);
		strFilterNum = StringOf(i+1);

		std::string  strFilterNameValue = SUBKEY_FilterName + strFilterNum;
		std::string  strName = reg.GetString( strKeyPath, strFilterNameValue, "Unnamed" );
		if (strName == "") strName = "Position " + StringOf(i+1); //strName.Format("Position %d", i+1);
			
		filter.Name = std::string( strName );

		std::string  strFilterOffsetValue = SUBKEY_FilterFocus + strFilterNum;
		filter.Offset = reg.GetNumber( strKeyPath, strFilterOffsetValue, 0 );

		std::string  strFilterTrimValue = SUBKEY_FilterTrim + strFilterNum;
		filter.Trim = reg.GetNumber( strKeyPath, strFilterTrimValue, 0 );

		this->Filters.push_back(filter);
	}

	// Now check to see if we need to expand the wheel size to match the camera's current wheel.
	if (iNumFiltersExpected > m_iNumFilters)
	{
		Filter filter;
		for (int i = m_iNumFilters; i < iNumFiltersExpected; i++)
		{
			AddFilter( filter );
		}
	}
	
	return true;
}

void FilterWheel::AddFilter(Filter filter)
{
	Filters.push_back(filter);
	m_iNumFilters++;
}

void FilterWheel::SaveToRegistry(std::string  strSerialNumber)
{
	QSI_Registry reg;
	std::string strValue;
	std::string strWheelName = this->Name;
	std::string strWheelNames = KEY_QSI  + strSerialNumber + "/FilterWheel/Names";
	reg.SetString(strWheelNames, Name.c_str(), "");

	std::string strKeyPath = KEY_QSI  + strSerialNumber + "/FilterWheels/" + this->Name.c_str();

	// Save filter count, no all position maybe named (below)
	reg.SetNumber(strKeyPath, SUBKEY_NumFilters, m_iNumFilters);

	for (int i = 0; i < m_iNumFilters; i++)
	{
		std::string strFilterNum;
		//strFilterNum.Format("%d", i+1);
		strFilterNum = StringOf(i+1);

		std::string strFilterNameValue = SUBKEY_FilterName + strFilterNum;
		strValue = this->Filters[i].Name.c_str();
		reg.SetString( strKeyPath, strFilterNameValue, strValue );

		std::string strFilterOffsetValue = SUBKEY_FilterFocus + strFilterNum;
		reg.SetNumber( strKeyPath, strFilterOffsetValue, this->Filters[i].Offset);

		std::string  strFilterTrimValue = SUBKEY_FilterTrim + strFilterNum;
		reg.SetNumber( strKeyPath, strFilterTrimValue, this->Filters[i].Trim);
	}
	return;
}

void FilterWheel::DeleteFromRegistry(std::string strSerialNumber)
{
	QSI_Registry reg;

	std::string strWheelNames = KEY_QSI  + strSerialNumber + "/FilterWheel/Names";
	reg.RegDelKey(strWheelNames, Name);

	std::string strKeyPath = KEY_QSI  + strSerialNumber + _T("/FilterWheels/");
	reg.RegDelnode (strKeyPath + this->Name);
}
