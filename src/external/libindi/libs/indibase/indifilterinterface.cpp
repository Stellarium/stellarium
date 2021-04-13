/*
    Filter Interface
    Copyright (C) 2011 Jasem Mutlaq (mutlaqja@ikarustech.com)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

*/

#include "indifilterinterface.h"
#include <cstring>
#include "indilogger.h"

namespace INDI
{

FilterInterface::FilterInterface(DefaultDevice *defaultDevice) : m_defaultDevice(defaultDevice)
{
    FilterNameTP = new ITextVectorProperty;
    FilterNameT  = nullptr;
}

FilterInterface::~FilterInterface()
{
    delete FilterNameTP;
}

void FilterInterface::initProperties(const char *groupName)
{
    IUFillNumber(&FilterSlotN[0], "FILTER_SLOT_VALUE", "Filter", "%3.0f", 1.0, 12.0, 1.0, 1.0);
    IUFillNumberVector(&FilterSlotNP, FilterSlotN, 1, m_defaultDevice->getDeviceName(), "FILTER_SLOT", "Filter Slot", groupName, IP_RW, 60,
                       IPS_IDLE);
}

bool FilterInterface::updateProperties()
{
    if (m_defaultDevice->isConnected())
    {
        // Define the Filter Slot and name properties
        m_defaultDevice->defineNumber(&FilterSlotNP);
        if (FilterNameT == nullptr)
        {
            if (GetFilterNames() == true)
                m_defaultDevice->defineText(FilterNameTP);
        }
        else
            m_defaultDevice->defineText(FilterNameTP);
    }
    else
    {
        m_defaultDevice->deleteProperty(FilterSlotNP.name);
        m_defaultDevice->deleteProperty(FilterNameTP->name);
    }

    return true;
}

bool FilterInterface::processNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    INDI_UNUSED(n);

    if (dev && !strcmp(dev, m_defaultDevice->getDeviceName()) && !strcmp(name, FilterSlotNP.name))
    {
        TargetFilter = values[0];

        INumber *np = IUFindNumber(&FilterSlotNP, names[0]);

        if (!np)
        {
            FilterSlotNP.s = IPS_ALERT;
            DEBUGFDEVICE(m_defaultDevice->getDeviceName(), Logger::DBG_ERROR, "Unknown error. %s is not a member of %s property.", names[0], FilterSlotNP.name);
            IDSetNumber(&FilterSlotNP, nullptr);
            return false;
        }

        if (TargetFilter < FilterSlotN[0].min || TargetFilter > FilterSlotN[0].max)
        {
            FilterSlotNP.s = IPS_ALERT;
            DEBUGFDEVICE(m_defaultDevice->getDeviceName(), Logger::DBG_ERROR, "Error: valid range of filter is from %g to %g", FilterSlotN[0].min, FilterSlotN[0].max);
            IDSetNumber(&FilterSlotNP, nullptr);
            return false;
        }

        FilterSlotNP.s = IPS_BUSY;
        DEBUGFDEVICE(m_defaultDevice->getDeviceName(), Logger::DBG_SESSION, "Setting current filter to slot %d", TargetFilter);

        if (SelectFilter(TargetFilter) == false)
        {
            FilterSlotNP.s = IPS_ALERT;
        }

        IDSetNumber(&FilterSlotNP, nullptr);
        return true;
    }

    return false;
}

bool FilterInterface::processText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    if (dev && !strcmp(dev, m_defaultDevice->getDeviceName()) && !strcmp(name, "FILTER_NAME"))
    {
        // If this call due to config loading, let's delete existing dummy property and define the full one
        if (loadingFromConfig)
        {
            loadingFromConfig = false;
            m_defaultDevice->deleteProperty("FILTER_NAME");

            char filterName[MAXINDINAME];
            char filterLabel[MAXINDILABEL];

            if (FilterNameT != nullptr)
                delete [] FilterNameT;

            FilterNameT = new IText[n];

            for (int i = 0; i < n; i++)
            {
                snprintf(filterName, MAXINDINAME, "FILTER_SLOT_NAME_%d", i + 1);
                snprintf(filterLabel, MAXINDILABEL, "Filter#%d", i + 1);
                IUFillText(&FilterNameT[i], filterName, filterLabel, texts[i]);
            }

            IUFillTextVector(FilterNameTP, FilterNameT, n, m_defaultDevice->getDeviceName(), "FILTER_NAME", "Filter", FilterSlotNP.group, IP_RW, 0, IPS_IDLE);
            m_defaultDevice->defineText(FilterNameTP);
            return true;
        }

        IUUpdateText(FilterNameTP, texts, names, n);
        FilterNameTP->s = IPS_OK;

        if (SetFilterNames() == true)
        {
            IDSetText(FilterNameTP, nullptr);
            return true;
        }
        else
        {
            FilterNameTP->s = IPS_ALERT;
            DEBUGDEVICE(m_defaultDevice->getDeviceName(), Logger::DBG_ERROR, "Error updating names of filters.");
            IDSetText(FilterNameTP, nullptr);
            return false;
        }
    }

    return false;
}

bool FilterInterface::saveConfigItems(FILE *fp)
{
    IUSaveConfigNumber(fp, &FilterSlotNP);
    if (FilterNameTP)
        IUSaveConfigText(fp, FilterNameTP);

    return true;
}

void FilterInterface::SelectFilterDone(int f)
{
    //  The hardware has finished changing
    //  filters
    FilterSlotN[0].value = f;
    FilterSlotNP.s       = IPS_OK;
    // Tell the clients we are done, and
    //  filter is now useable
    IDSetNumber(&FilterSlotNP, nullptr);
}

void FilterInterface::generateSampleFilters()
{
    char filterName[MAXINDINAME];
    char filterLabel[MAXINDILABEL];
    int MaxFilter = FilterSlotN[0].max;

    const char *filterDesignation[8] = { "Red", "Green", "Blue", "H_Alpha", "SII", "OIII", "LPR", "Luminance" };

    if (FilterNameT != nullptr)
        delete [] FilterNameT;

    FilterNameT = new IText[MaxFilter];

    for (int i = 0; i < MaxFilter; i++)
    {
        snprintf(filterName, MAXINDINAME, "FILTER_SLOT_NAME_%d", i + 1);
        snprintf(filterLabel, MAXINDILABEL, "Filter#%d", i + 1);
        IUFillText(&FilterNameT[i], filterName, filterLabel, i < 8 ? filterDesignation[i] : filterLabel);
    }

    IUFillTextVector(FilterNameTP, FilterNameT, MaxFilter, m_defaultDevice->getDeviceName(), "FILTER_NAME", "Filter", FilterSlotNP.group, IP_RW, 0, IPS_IDLE);
}

bool FilterInterface::GetFilterNames()
{
    // Load from config
    if (FilterNameT == nullptr)
    {
        generateSampleFilters();
        // If property is found, let's define it once loaded to the client and delete
        // the generate sample filters above
        if (m_defaultDevice->loadConfig(true, "FILTER_NAME"))
            loadingFromConfig = true;
    }

    return true;
}

bool FilterInterface::SetFilterNames()
{
    return m_defaultDevice->saveConfig(true, "FILTER_NAME");
}
}
