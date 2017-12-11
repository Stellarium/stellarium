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

#pragma once

#include <string>
#include <vector>
#include "indibase.h"

/**
 * \class FilterInterface
   \brief Provides interface to implement Filter Wheel functionality.

   A filter wheel can be an independent device, or an embedded filter wheel within another device (e.g. CCD camera). Child class must implement all the
   pure virtual functions and call SelectFilterDone(int) when selection of a new filter position is complete in the hardware.

   \e IMPORTANT: initFilterProperties() must be called before any other function to initilize the filter properties.

   \e IMPORTANT: processFilterSlot() must be called in your driver's ISNewNumber() function. processFilterSlot() will call the driver's
      SelectFilter() accordingly.

   \note Filter position starts from 1 and \e not 0
\author Gerry Rozema, Jasem Mutlaq
*/
namespace INDI
{

class FilterInterface
{
  public:
    /** \brief Return current filter position */
    virtual int QueryFilter() = 0;

    /**
     * \brief Select a new filter position
     * \return True if operation is successful, false otherwise
     */
    virtual bool SelectFilter(int position) = 0;

    /**
     * \brief Set filter names as defined by the client for each filter position.
     * The desired filter names are stored in FilterNameTP property. Filter names should be
     * saved in hardware if possible. The default implementation saves them in the configuration file.
     * \return True if successful, false if supported or failed operation
     */
    virtual bool SetFilterNames();

    /**
     * \brief Obtains a list of filter names from the hardware and initializes the FilterNameTP
     * property. The function should check for the number of filters available in the filter
     * wheel and build the FilterNameTP property accordingly. The default implementation loads the filter names from
     * configuration file.
     * \return True if successful, false if unsupported or failed operation     
     */
    virtual bool GetFilterNames();

    /**
     * \brief The child class calls this function when the hardware successfully finished
     * selecting a new filter wheel position
     * \param newpos New position of the filter wheel
     */
    void SelectFilterDone(int newpos);

  protected:
    /**
     * @brief FilterInterface Initiailize Filter Interface
     * @param defaultDevice default device that owns the interface
     */
    explicit FilterInterface(DefaultDevice *defaultDevice);
    ~FilterInterface();

    /**
     * \brief Initilize filter wheel properties. It is recommended to call this function within
     * initProperties() of your primary device
     * \param groupName Group or tab name to be used to define filter wheel properties.
     */
    void initProperties(const char *groupName);

    /**
     * @brief updateProperties Defines or Delete proprties based on default device connection status
     * @return True if all is OK, false otherwise.
     */
    bool updateProperties();

    /** \brief Process number properties */
    bool processNumber(const char *dev, const char *name, double values[], char *names[], int n);

    /** \brief Process text properties */
    bool processText(const char *dev, const char *name, char *texts[], char *names[], int n);

    /**
     * @brief generateSampleFilters Generate sample 8-filter wheel and fill it sample filters
     */
    void generateSampleFilters();

    /**
     * @brief saveConfigItems save Filter Names in config file
     * @param fp pointer to config file
     * @return Always return true
     */
    bool saveConfigItems(FILE *fp);

    //  A number vector for filter slot
    INumberVectorProperty FilterSlotNP;
    INumber FilterSlotN[1];

     //  A text vector that stores out physical port name
    ITextVectorProperty *FilterNameTP { nullptr };
    IText *FilterNameT;

    int CurrentFilter;
    int TargetFilter;
    bool loadingFromConfig = false;

    DefaultDevice *m_defaultDevice { nullptr };
};
}
