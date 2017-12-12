/*
    Light Box / Switch Interface
    Copyright (C) 2015 Jasem Mutlaq (mutlaqja@ikarustech.com)

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

#include "indibase.h"

#include <stdint.h>

/**
 * \class LightBoxInterface
   \brief Provides interface to implement controllable light box/switch device.

   Filter durations preset can be defined if the active filter name is set. Once the filter names are retrieved, the duration in seconds can be set for each filter.
   When the filter wheel changes to a new filter, the duration is set accordingly.

   The child class is expected to call the following functions from the INDI frameworks standard functions:

   \e IMPORTANT: initLightBoxProperties() must be called before any other function to initilize the Light device properties.
   \e IMPORTANT: isGetLightBoxProperties() must be called in your driver ISGetProperties function
   \e IMPORTANT: processLightBoxSwitch() must be called in your driver ISNewSwitch function.
   \e IMPORTANT: processLightBoxNumber() must be called in your driver ISNewNumber function.
   \e IMPORTANT: processLightBoxText() must be called in your driver ISNewText function.
\author Jasem Mutlaq
*/
namespace INDI
{

class LightBoxInterface
{
  public:
    enum
    {
        FLAT_LIGHT_ON,
        FLAT_LIGHT_OFF
    };

  protected:
    LightBoxInterface(DefaultDevice *device, bool isDimmable);
    virtual ~LightBoxInterface();

    /** \brief Initilize light box properties. It is recommended to call this function within initProperties() of your primary device
            \param deviceName Name of the primary device
            \param groupName Group or tab name to be used to define light box properties.
        */
    void initLightBoxProperties(const char *deviceName, const char *groupNam);

    /**
         * @brief isGetLightBoxProperties Get light box properties
         * @param deviceName parent device name
         */
    void isGetLightBoxProperties(const char *deviceName);

    /** \brief Process light box switch properties */
    bool processLightBoxSwitch(const char *dev, const char *name, ISState *states, char *names[], int n);

    /** \brief Process light box number properties */
    bool processLightBoxNumber(const char *dev, const char *name, double values[], char *names[], int n);

    /** \brief Process light box text properties */
    bool processLightBoxText(const char *dev, const char *name, char *texts[], char *names[], int n);

    bool updateLightBoxProperties();
    bool saveLightBoxConfigItems(FILE *fp);
    bool snoopLightBox(XMLEle *root);

    /**
         * @brief setBrightness Set light level. Must be impelemented in the child class, if supported.
         * @param value level of light box
         * @return True if successful, false otherwise.
         */
    virtual bool SetLightBoxBrightness(uint16_t value);

    /**
         * @brief EnableLightBox Turn on/off on a light box. Must be impelemented in the child class.
         * @param enable If true, turn on the light, otherwise turn off the light.
         * @return True if successful, false otherwise.
         */
    virtual bool EnableLightBox(bool enable);

    // Turn on/off light
    ISwitchVectorProperty LightSP;
    ISwitch LightS[2];

    // Light Intensity
    INumberVectorProperty LightIntensityNP;
    INumber LightIntensityN[1];

    // Active devices to snoop
    ITextVectorProperty ActiveDeviceTP;
    IText ActiveDeviceT[1];

    INumberVectorProperty FilterIntensityNP;
    INumber *FilterIntensityN;

  private:
    void addFilterDuration(const char *filterName, uint16_t filterDuration);

    DefaultDevice *device;
    uint8_t currentFilterSlot;
    bool isDimmable;
};
}
