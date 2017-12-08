#if 0
    Induino general propose driver. Allow using arduino boards
    as general I/O 	
    Copyright 2012 (c) Nacho Mas (mas.ignacio at gmail.com)


    Base on Tutorial Four
    Demonstration of libindi v0.7 capabilities.

    Copyright (C) 2010 Jasem Mutlaq (mutlaqja@ikarustech.com)

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

#endif

#include "indiduino.h"

#include "config.h"
#include "firmata.h"

#include <indicontroller.h>

#include <memory>
#include <sys/stat.h>

/* Our indiduino auto pointer */
std::unique_ptr<indiduino> indiduino_prt(new indiduino());

const int POLLMS = 500; // Period of update, 1 second.

/**************************************************************************************
** Send client definitions of all properties.
***************************************************************************************/
/**************************************************************************************
**
***************************************************************************************/
void ISGetProperties(const char *dev)
{
    indiduino_prt->ISGetProperties(dev);
}

/**************************************************************************************
**
***************************************************************************************/
void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    indiduino_prt->ISNewSwitch(dev, name, states, names, n);
}

/**************************************************************************************
**
***************************************************************************************/
void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    indiduino_prt->ISNewText(dev, name, texts, names, n);
}

/**************************************************************************************
**
***************************************************************************************/
void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    indiduino_prt->ISNewNumber(dev, name, values, names, n);
}

/**************************************************************************************
**
***************************************************************************************/
void ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *formats[],
               char *names[], int n)
{
    indiduino_prt->ISNewBLOB(dev, name, sizes, blobsizes, blobs, formats, names, n);
}

/**************************************************************************************
**
***************************************************************************************/
void ISSnoopDevice(XMLEle *root)
{
    indiduino_prt->ISSnoopDevice(root);
}

/**************************************************************************************
**
***************************************************************************************/
indiduino::indiduino()
{
    DEBUG(INDI::Logger::DBG_DEBUG, "Indiduino driver start...");
    setVersion(DUINO_VERSION_MAJOR, DUINO_VERSION_MINOR);
    controller = new INDI::Controller(this);
    controller->setJoystickCallback(joystickHelper);
    controller->setButtonCallback(buttonHelper);
    controller->setAxisCallback(axisHelper);
}

/**************************************************************************************
**
***************************************************************************************/
indiduino::~indiduino()
{
    delete (controller);
}

bool indiduino::ISSnoopDevice(XMLEle *root)
{
    controller->ISSnoopDevice(root);
    return INDI::DefaultDevice::ISSnoopDevice(root);
}

void indiduino::TimerHit()
{
    if (isConnected() == false)
        return;

    sf->OnIdle();

    std::vector<INDI::Property *> *pAll = getProperties();

    for (unsigned int i = 0; i < pAll->size(); i++)
    {
        const char *name = pAll->at(i)->getName();
        INDI_PROPERTY_TYPE type = pAll->at(i)->getType();

        //DIGITAL INPUT
        if (type == INDI_LIGHT)
        {
            ILightVectorProperty *lvp = getLight(name);
            for (int i = 0; i < lvp->nlp; i++)
            {
                ILight *lqp = &lvp->lp[i];
                if (!lqp)
                    return;

                IO *pin_config = (IO *)lqp->aux;
                if (pin_config == NULL)
                    continue;
                if (pin_config->IOType == DI)
                {
                    int pin = pin_config->pin;
                    if (sf->pin_info[pin].mode == FIRMATA_MODE_INPUT)
                    {
                        if (sf->pin_info[pin].value == 1)
                        {
                            //IDLog("%s.%s on pin %u change to  ON\n",lvp->name,lqp->name,pin);
                            //IDSetLight (lvp, "%s.%s change to ON\n",lvp->name,lqp->name);
                            lqp->s = IPS_OK;
                        }
                        else
                        {
                            //IDLog("%s.%s on pin %u change to  OFF\n",lvp->name,lqp->name,pin);
                            //IDSetLight (lvp, "%s.%s change to OFF\n",lvp->name,lqp->name);
                            lqp->s = IPS_IDLE;
                        }
                    }
                }
            }
            IDSetLight(lvp, NULL);
        }

        //ANALOG
        if (type == INDI_NUMBER)
        {
            INumberVectorProperty *nvp = getNumber(name);

            for (int i = 0; i < nvp->nnp; i++)
            {
                INumber *eqp = &nvp->np[i];
                if (!eqp)
                    return;

                IO *pin_config = (IO *)eqp->aux0;
                if (pin_config == NULL)
                    continue;

                if (pin_config->IOType == AI)
                {
                    int pin = pin_config->pin;
                    if (sf->pin_info[pin].mode == FIRMATA_MODE_ANALOG)
                    {
                        eqp->value = pin_config->MulScale * (double)(sf->pin_info[pin].value) + pin_config->AddScale;
                        //IDLog("%f\n",eqp->value);
                        IDSetNumber(nvp, NULL);
                    }
                }
            }
        }

        //TEXT
        //if (type == INDI_TEXT) {
        //  ITextVectorProperty *tvp = getText(name);

        //for (int i=0;i<tvp->ntp;i++) {
        //    	IText *eqp = &tvp->tp[i];
        //  		if (!eqp)
        //    	    return;

        //if (eqp->aux0 == NULL) continue;
        //			strcpy(eqp->text,(char*)eqp->aux0);
        //IDLog("%s.%s TEXT: %s \n",tvp->name,eqp->name,eqp->text);
        //	IDSetText(tvp, NULL);
        //	}
        //	}
    }

    SetTimer(POLLMS);
}

/**************************************************************************************
** Initialize all properties & set default values.
**************************************************************************************/
bool indiduino::initProperties()
{
    // This is the default driver skeleton file location
    // Convention is: drivername_sk_xml
    // Default location is /usr/share/indi

    struct stat st;

    strcpy(skelFileName, DEFAULT_SKELETON_FILE);

    char *skel = getenv("INDISKEL");
    if (skel)
    {
        IDLog("Building from %s skeleton\n", skel);
        strcpy(skelFileName, skel);
        buildSkeleton(skel);
    }
    else if (stat(skelFileName, &st) == 0)
    {
        IDLog("Building from %s skeleton\n", skelFileName);
        buildSkeleton(skelFileName);
    }
    else
    {
        IDLog(
            "No skeleton file was specified. Set environment variable INDISKEL to the skeleton path and try again.\n");
    }
    // Optional: Add aux controls for configuration, debug & simulation that get added in the Options tab
    //           of the driver.
    //addAuxControls();

    controller->initProperties();

    DefaultDevice::initProperties();

    return true;
}

bool indiduino::updateProperties()
{
    if (isConnected())
    {
        // Mapping the controller according to the properties previously read from the XML file
        // We only map controls for pin of type AO and SERVO
        for (int numiopin = 0; numiopin < MAX_IO_PIN; numiopin++)
        {
            if (iopin[numiopin].IOType == SERVO)
            {
                if (iopin[numiopin].SwitchButton)
                {
                    char buffer[33];
                    sprintf(buffer, "%d", numiopin);
                    controller->mapController(buffer, iopin[numiopin].defVectorName,
                                              INDI::Controller::CONTROLLER_BUTTON, iopin[numiopin].SwitchButton);
                }
            }
            else if (iopin[numiopin].IOType == AO)
            {
                if (iopin[numiopin].UpButton && iopin[numiopin].DownButton)
                {
                    char upBuffer[33];
                    sprintf(upBuffer, "%d", numiopin);
                    // To distinguish the up button from the down button, we add MAX_IO_PIN to the message
                    char downBuffer[33];
                    sprintf(downBuffer, "%d", numiopin + MAX_IO_PIN);
                    controller->mapController(upBuffer, iopin[numiopin].defVectorName,
                                              INDI::Controller::CONTROLLER_BUTTON, iopin[numiopin].UpButton);
                    controller->mapController(downBuffer, iopin[numiopin].defVectorName,
                                              INDI::Controller::CONTROLLER_BUTTON, iopin[numiopin].DownButton);
                }
            }
        }
    }
    controller->updateProperties();
    return true;
}

/**************************************************************************************
** Define Basic properties to clients.
***************************************************************************************/
void indiduino::ISGetProperties(const char *dev)
{
    static int configLoaded = 0;

    // Ask the default driver first to send properties.
    INDI::DefaultDevice::ISGetProperties(dev);

    configLoaded = 1;

    controller->ISGetProperties(dev);
}

/**************************************************************************************
** Process Text properties
***************************************************************************************/
bool indiduino::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    // Ignore if not ours
    if (strcmp(dev, getDeviceName()))
        return false;

    ITextVectorProperty *tProp = getText(name);
    // Device Port Text
    if (!strcmp(tProp->name, "DEVICE_PORT"))
    {
        if (IUUpdateText(tProp, texts, names, n) < 0)
            return false;

        tProp->s = IPS_OK;
        tProp->s = IPS_IDLE;
        IDSetText(tProp, "Port updated.");

        return true;
    }

    controller->ISNewText(dev, name, texts, names, n);

    return false;
}

/**************************************************************************************
**
***************************************************************************************/
bool indiduino::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    // Ignore if not ours
    if (strcmp(dev, getDeviceName()))
        return false;

    INumberVectorProperty *nvp = getNumber(name);
    if (!nvp)
        return false;

    if (isConnected() == false)
    {
        nvp->s = IPS_ALERT;
        IDSetNumber(nvp, "Cannot change property while device is disconnected.");
        return false;
    }

    bool change = false;
    for (int i = 0; i < n; i++)
    {
        INumber *eqp = IUFindNumber(nvp, names[i]);

        if (!eqp)
            return false;

        IO *pin_config = (IO *)eqp->aux0;
        if (pin_config == NULL)
            continue;
        if ((pin_config->IOType == AO) || (pin_config->IOType == SERVO))
        {
            int pin = pin_config->pin;
            IUUpdateNumber(nvp, values, names, n);
            IDLog("Setting output %s.%s on pin %u to %f\n", nvp->name, eqp->name, pin, eqp->value);
            sf->setPwmPin(pin, (int)(pin_config->MulScale * eqp->value + pin_config->AddScale));
            IDSetNumber(nvp, "%s.%s change to %f\n", nvp->name, eqp->name, eqp->value);
            nvp->s = IPS_IDLE;
            change = true;
        }
        if (pin_config->IOType == AI)
        {
            IUUpdateNumber(nvp, values, names, n);
            nvp->s = IPS_IDLE;
            change = true;
        }
    }

    if (change)
    {
        IDSetNumber(nvp, NULL);
        return true;
    }
    else
    {
        return false;
    }
}

/**************************************************************************************
**
***************************************************************************************/
bool indiduino::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    for (int i = 0; i < n; i++)
    {
        if (states[i] == ISS_ON)
            IDLog("State %d is on\n", i);
        else if (states[i] == ISS_OFF)
            IDLog("State %d is off\n", i);
    }
    // ignore if not ours //
    if (strcmp(dev, getDeviceName()))
        return false;

    if (INDI::DefaultDevice::ISNewSwitch(dev, name, states, names, n) == true)
        return true;

    ISwitchVectorProperty *svp = getSwitch(name);

    if (isConnected() == false)
    {
        svp->s = IPS_ALERT;
        IDSetSwitch(svp, "Cannot change property while device is disconnected.");
        return false;
    }

    if (!svp)
        return false;

    for (int i = 0; i < svp->nsp; i++)
    {
        ISwitch *sqp   = &svp->sp[i];
        IO *pin_config = (IO *)sqp->aux;
        if (pin_config == NULL)
            continue;
        if (pin_config->IOType == DO)
        {
            int pin = pin_config->pin;
            IUUpdateSwitch(svp, states, names, n);
            if (sqp->s == ISS_ON)
            {
                IDLog("Switching ON %s.%s on pin %u\n", svp->name, sqp->name, pin);
                sf->writeDigitalPin(pin, ARDUINO_HIGH);
                IDSetSwitch(svp, "%s.%s ON\n", svp->name, sqp->name);
            }
            else
            {
                IDLog("Switching OFF %s.%s on pin %u\n", svp->name, sqp->name, pin);
                sf->writeDigitalPin(pin, ARDUINO_LOW);
                IDSetSwitch(svp, "%s.%s OFF\n", svp->name, sqp->name);
            }
        }
        else if (pin_config->IOType == SERVO)
        {
            int pin = pin_config->pin;
            IUUpdateSwitch(svp, states, names, n);
            if (sqp->s == ISS_ON)
            {
                IDLog("Switching ON %s.%s on pin %u\n", svp->name, sqp->name, pin);
                sf->setPwmPin(pin, (int)pin_config->OnAngle);
                IDSetSwitch(svp, "%s.%s ON\n", svp->name, sqp->name);
            }
            else
            {
                IDLog("Switching OFF %s.%s on pin %u\n", svp->name, sqp->name, pin);
                sf->setPwmPin(pin, (int)pin_config->OffAngle);
                IDSetSwitch(svp, "%s.%s OFF\n", svp->name, sqp->name);
            }
        }
    }
    controller->ISNewSwitch(dev, name, states, names, n);

    IUUpdateSwitch(svp, states, names, n);
    return true;
}

bool indiduino::ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[],
                          char *formats[], char *names[], int n)
{
    if (strcmp(dev, getDeviceName()))
        return false;

    IBLOBVectorProperty *bvp = getBLOB(name);

    if (!bvp)
        return false;

    if (isConnected() == false)
    {
        bvp->s = IPS_ALERT;
        IDSetBLOB(bvp, "Cannot change property while device is disconnected.");
        return false;
    }

    if (!strcmp(bvp->name, "BLOB Test"))
    {
        IUUpdateBLOB(bvp, sizes, blobsizes, blobs, formats, names, n);

        IBLOB *bp = IUFindBLOB(bvp, names[0]);

        if (!bp)
            return false;

        IDLog("Recieved BLOB with name %s, format %s, and size %d, and bloblen %d\n", bp->name, bp->format, bp->size,
              bp->bloblen);

        char *blobBuffer = new char[bp->bloblen + 1];
        strncpy(blobBuffer, ((char *)bp->blob), bp->bloblen);
        blobBuffer[bp->bloblen] = '\0';

        IDLog("BLOB Content:\n##################################\n%s\n##################################\n",
              blobBuffer);

        delete[] blobBuffer;
    }

    return true;
}

/**************************************************************************************
**
***************************************************************************************/
bool indiduino::Connect()
{
    ITextVectorProperty *tProp = getText("DEVICE_PORT");
    sf                         = new Firmata(tProp->tp[0].text);
    if (sf->portOpen)
    {
        IDLog("ARDUINO BOARD CONNECTED.\n");
        IDLog("FIRMATA VERSION:%s\n", sf->firmata_name);
        IDSetSwitch(getSwitch("CONNECTION"), "CONNECTED.FIRMATA VERSION:%s\n", sf->firmata_name);
        if (!setPinModesFromSKEL())
        {
            IDLog("FAIL TO MAP ARDUINO PINS. CHECK SKELETON FILE SINTAX\n");
            IDSetSwitch(getSwitch("CONNECTION"), "FAIL TO MAP ARDUINO PINS. CHECK SKELETON FILE SINTAX\n");
            delete sf;
            return false;
        }
        else
        {
            SetTimer(POLLMS);
            return true;
        }
    }
    else
    {
        IDLog("ARDUINO BOARD FAIL TO CONNECT. CHECK PORT NAME\n");
        IDSetSwitch(getSwitch("CONNECTION"), "ARDUINO BOARD FAIL TO CONNECT. CHECK PORT NAME\n");
        delete sf;
        return false;
    }
}

bool indiduino::Disconnect()
{
    sf->closePort();
    delete sf;
    IDLog("ARDUINO BOARD DISCONNECTED.\n");
    IDSetSwitch(getSwitch("CONNECTION"), "DISCONNECTED\n");
    return true;
}

const char *indiduino::getDefaultName()
{
    return "Arduino";
}

bool indiduino::setPinModesFromSKEL()
{
    char errmsg[MAXRBUF];
    FILE *fp       = NULL;
    LilXML *lp     = newLilXML();
    XMLEle *fproot = NULL;
    XMLEle *ep = NULL, *ioep = NULL, *xmlp = NULL;
    int numiopin = 0;

    fp = fopen(skelFileName, "r");

    if (fp == NULL)
    {
        IDLog("Unable to build skeleton. Error loading file %s: %s\n", skelFileName, strerror(errno));
        return false;
    }

    fproot = readXMLFile(fp, lp, errmsg);

    if (fproot == NULL)
    {
        IDLog("Unable to parse skeleton XML: %s", errmsg);
        return false;
    }

    IDLog("Setting pins behaviour from <indiduino> tags\n");
    std::vector<INDI::Property *> *pAll = getProperties();

    for (unsigned int i = 0; i < pAll->size(); i++)
    {
        const char *name = pAll->at(i)->getName();
        INDI_PROPERTY_TYPE type = pAll->at(i)->getType();

        if (ep == NULL)
        {
            ep = nextXMLEle(fproot, 1);
        }
        else
        {
            ep = nextXMLEle(fproot, 0);
        }
        if (ep == NULL)
        {
            break;
        }
        ioep = NULL;
        if (type == INDI_SWITCH)
        {
            ISwitchVectorProperty *svp = getSwitch(name);
            ioep = NULL;
            for (int i = 0; i < svp->nsp; i++)
            {
                ISwitch *sqp = &svp->sp[i];
                if (ioep == NULL)
                {
                    ioep = nextXMLEle(ep, 1);
                }
                else
                {
                    ioep = nextXMLEle(ep, 0);
                }
                xmlp = findXMLEle(ioep, "indiduino");
                if (xmlp != NULL)
                {
                    if (!readInduinoXml(xmlp, numiopin))
                    {
                        IDLog("Malforme <indiduino> XML\n");
                        return false;
                    }
                    sqp->aux                      = (void *)&iopin[numiopin];
                    iopin[numiopin].defVectorName = svp->name;
                    iopin[numiopin].defName       = sqp->name;
                    int pin                       = iopin[numiopin].pin;
                    if (iopin[numiopin].IOType == DO)
                    {
                        IDLog("%s.%s  pin %u set as DIGITAL OUTPUT\n", svp->name, sqp->name, pin);
                        sf->setPinMode(pin, FIRMATA_MODE_OUTPUT);
                    }
                    else if (iopin[numiopin].IOType == SERVO)
                    {
                        IDLog("%s.%s  pin %u set as SERVO\n", svp->name, sqp->name, pin);
                        sf->setPinMode(pin, FIRMATA_MODE_SERVO);
                        // Setting Servo pin to default startup angle
                        sf->setPwmPin(
                            pin, (int)(iopin[numiopin].MulScale * iopin[numiopin].OnAngle + iopin[numiopin].AddScale));
                    }
                    IDLog("numiopin:%u\n", numiopin);
                    numiopin++;
                }
            }
        }
        if (type == INDI_TEXT)
        {
            ITextVectorProperty *tvp = getText(name);

            ioep = NULL;
            for (int i = 0; i < tvp->ntp; i++)
            {
                IText *tqp = &tvp->tp[i];

                if (ioep == NULL)
                {
                    ioep = nextXMLEle(ep, 1);
                }
                else
                {
                    ioep = nextXMLEle(ep, 0);
                }
                xmlp = findXMLEle(ioep, "indiduino");
                if (xmlp != NULL)
                {
                    if (!readInduinoXml(xmlp, 0))
                    {
                        IDLog("Malforme <indiduino> XML\n");
                        return false;
                    }
                    tqp->aux0                     = (void *)&sf->string_buffer;
                    iopin[numiopin].defVectorName = tvp->name;
                    iopin[numiopin].defName       = tqp->name;
                    IDLog("%s.%s ARDUINO TEXT\n", tvp->name, tqp->name);
                    IDLog("numiopin:%u\n", numiopin);
                }
            }
        }
        if (type == INDI_LIGHT)
        {
            ILightVectorProperty *lvp = getLight(name);

            ioep = NULL;
            for (int i = 0; i < lvp->nlp; i++)
            {
                ILight *lqp = &lvp->lp[i];
                if (ioep == NULL)
                {
                    ioep = nextXMLEle(ep, 1);
                }
                else
                {
                    ioep = nextXMLEle(ep, 0);
                }
                xmlp = findXMLEle(ioep, "indiduino");
                if (xmlp != NULL)
                {
                    if (!readInduinoXml(xmlp, numiopin))
                    {
                        IDLog("Malforme <indiduino> XML\n");
                        return false;
                    }
                    lqp->aux                      = (void *)&iopin[numiopin];
                    iopin[numiopin].defVectorName = lvp->name;
                    iopin[numiopin].defName       = lqp->name;
                    int pin                       = iopin[numiopin].pin;
                    IDLog("%s.%s  pin %u set as DIGITAL INPUT\n", lvp->name, lqp->name, pin);
                    sf->setPinMode(pin, FIRMATA_MODE_INPUT);
                    IDLog("numiopin:%u\n", numiopin);
                    numiopin++;
                }
            }
        }
        if (type == INDI_NUMBER)
        {
            INumberVectorProperty *nvp = getNumber(name);

            ioep = NULL;
            for (int i = 0; i < nvp->nnp; i++)
            {
                INumber *eqp = &nvp->np[i];
                if (ioep == NULL)
                {
                    ioep = nextXMLEle(ep, 1);
                }
                else
                {
                    ioep = nextXMLEle(ep, 0);
                }
                xmlp = findXMLEle(ioep, "indiduino");
                if (xmlp != NULL)
                {
                    if (!readInduinoXml(xmlp, numiopin))
                    {
                        IDLog("Malforme <indiduino> XML\n");
                        return false;
                    }
                    eqp->aux0                     = (void *)&iopin[numiopin];
                    iopin[numiopin].defVectorName = nvp->name;
                    iopin[numiopin].defName       = eqp->name;
                    int pin                       = iopin[numiopin].pin;
                    if (iopin[numiopin].IOType == AO)
                    {
                        IDLog("%s.%s  pin %u set as ANALOG OUTPUT\n", nvp->name, eqp->name, pin);
                        sf->setPinMode(pin, FIRMATA_MODE_PWM);
                    }
                    else if (iopin[numiopin].IOType == AI)
                    {
                        IDLog("%s.%s  pin %u set as ANALOG INPUT\n", nvp->name, eqp->name, pin);
                        sf->setPinMode(pin, FIRMATA_MODE_ANALOG);
                    }
                    else if (iopin[numiopin].IOType == SERVO)
                    {
                        IDLog("%s.%s  pin %u set as SERVO\n", nvp->name, eqp->name, pin);
                        sf->setPinMode(pin, FIRMATA_MODE_SERVO);
                    }
                    IDLog("numiopin:%u\n", numiopin);
                    numiopin++;
                }
            }
        }
    }
    sf->setSamplingInterval(POLLMS / 2);
    sf->reportAnalogPorts(1);
    sf->reportDigitalPorts(1);
    return true;
}

bool indiduino::readInduinoXml(XMLEle *ioep, int npin)
{
    char *propertyTag;
    int pin;

    if (ioep == NULL)
        return false;

    propertyTag = tagXMLEle(parentXMLEle(ioep));

    if (!strcmp(propertyTag, "defSwitch") || !strcmp(propertyTag, "defLight") || !strcmp(propertyTag, "defNumber"))
    {
        pin = atoi(findXMLAttValu(ioep, "pin"));

        if (pin >= 1 && pin <= 40)
        { //19 hardware pins.
            iopin[npin].pin = pin;
        }
        else
        {
            IDLog("induino: pin number is required. Check pin attrib value (1-19)\n");
            return false;
        }

        if (!strcmp(propertyTag, "defSwitch"))
        {
            if (!strcmp(findXMLAttValu(ioep, "type"), "servo"))
            {
                iopin[npin].IOType = SERVO;
                if (strcmp(findXMLAttValu(ioep, "onangle"), ""))
                {
                    iopin[npin].OnAngle = atof(findXMLAttValu(ioep, "onangle"));
                }
                else
                {
                    iopin[npin].OnAngle = 150;
                }
                if (strcmp(findXMLAttValu(ioep, "offangle"), ""))
                {
                    iopin[npin].OffAngle = atof(findXMLAttValu(ioep, "offangle"));
                }
                else
                {
                    iopin[npin].OffAngle = 10;
                }
                if (strcmp(findXMLAttValu(ioep, "button"), ""))
                {
                    iopin[npin].SwitchButton = const_cast<char *>(findXMLAttValu(ioep, "button"));
                    IDLog("found button %s\n", iopin[npin].SwitchButton);
                }
            }
            else
            {
                iopin[npin].IOType = DO;
            }
        }

        if (!strcmp(propertyTag, "defLight"))
        {
            iopin[npin].IOType = DI;
        }

        if (!strcmp(propertyTag, "defNumber"))
        {
            if (strcmp(findXMLAttValu(ioep, "mul"), ""))
            {
                iopin[npin].MulScale = atof(findXMLAttValu(ioep, "mul"));
            }
            else
            {
                iopin[npin].MulScale = 1;
            }
            if (strcmp(findXMLAttValu(ioep, "add"), ""))
            {
                iopin[npin].AddScale = atof(findXMLAttValu(ioep, "add"));
            }
            else
            {
                iopin[npin].AddScale = 0;
            }
            if (!strcmp(findXMLAttValu(ioep, "type"), "output"))
            {
                iopin[npin].IOType = AO;
            }
            else if (!strcmp(findXMLAttValu(ioep, "type"), "input"))
            {
                iopin[npin].IOType = AI;
            }
            else if (!strcmp(findXMLAttValu(ioep, "type"), "servo"))
            {
                iopin[npin].IOType = SERVO;
            }
            else
            {
                IDLog("induino: Setting type (input or output) is required for analogs\n");
                return false;
            }
            if (strcmp(findXMLAttValu(ioep, "downbutton"), ""))
            {
                iopin[npin].DownButton = const_cast<char *>(findXMLAttValu(ioep, "downbutton"));
            }
            if (strcmp(findXMLAttValu(ioep, "upbutton"), ""))
            {
                iopin[npin].UpButton = const_cast<char *>(findXMLAttValu(ioep, "upbutton"));
            }
            if (strcmp(findXMLAttValu(ioep, "buttonincvalue"), ""))
            {
                iopin[npin].buttonIncValue = atof(findXMLAttValu(ioep, "buttonincvalue"));
            }
            else
            {
                iopin[npin].buttonIncValue = 50;
            }
        }

        if (false)
        {
            IDLog("arduino IO Match:Property:(%s)\n", findXMLAttValu(parentXMLEle(ioep), "name"));
            IDLog("arduino IO pin:(%u)\n", iopin[npin].pin);
            IDLog("arduino IO type:(%u)\n", iopin[npin].IOType);
            IDLog("arduino IO scale: mul:%g add:%g\n", iopin[npin].MulScale, iopin[npin].AddScale);
        }
    }
    return true;
}

void indiduino::joystickHelper(const char *joystick_n, double mag, double angle, void *context)
{
    static_cast<indiduino *>(context)->processJoystick(joystick_n, mag, angle);
}

void indiduino::buttonHelper(const char *button_n, ISState state, void *context)
{
    static_cast<indiduino *>(context)->processButton(button_n, state);
}

void indiduino::axisHelper(const char *axis_n, double value, void *context)
{
    static_cast<indiduino *>(context)->processAxis(axis_n, value);
}

void indiduino::processAxis(const char *axis_n, double value)
{
    INDI_UNUSED(axis_n);
    INDI_UNUSED(value);
    // TO BE IMPLEMENTED
}

void indiduino::processJoystick(const char *joystick_n, double mag, double angle)
{
    INDI_UNUSED(joystick_n);
    INDI_UNUSED(mag);
    INDI_UNUSED(angle);
    // TO BE IMPLEMENTED
}

void indiduino::processButton(const char *button_n, ISState state)
{
    //ignore OFF
    if (state == ISS_OFF)
        return;

    int numiopin;
    numiopin = atoi(button_n);

    bool isDownAO = false;
    // Recognizing a shifted numiopin means it is a button to decrease the value of a AO
    if (numiopin >= MAX_IO_PIN)
    {
        isDownAO = true;
        numiopin = numiopin - MAX_IO_PIN;
    }

    if (iopin[numiopin].IOType == AO)
    {
        char *names[1]             = { iopin[numiopin].defName };
        char *name                 = iopin[numiopin].defVectorName;
        INumberVectorProperty *nvp = getNumber(name);
        INumber *eqp               = IUFindNumber(nvp, names[0]);
        if (isDownAO)
        {
            double values[1] = { eqp->value - iopin[numiopin].buttonIncValue };
            ISNewNumber(getDeviceName(), name, values, names, 1);
        }
        else
        {
            double values[1] = { eqp->value + iopin[numiopin].buttonIncValue };
            ISNewNumber(getDeviceName(), name, values, names, 1);
        }
    }
    else if (iopin[numiopin].IOType == SERVO)
    {
        ISwitchVectorProperty *svp = getSwitch(iopin[numiopin].defVectorName);
        // Only considering the first switch, because the servo switches must be configured with only one switch
        ISwitch *sqp   = &svp->sp[0];
        char *names[1] = { iopin[numiopin].defName };
        if (sqp->s == ISS_ON)
        {
            ISState states[1] = { ISS_OFF };
            ISNewSwitch(getDeviceName(), iopin[numiopin].defVectorName, states, names, 1);
        }
        else
        {
            ISState states[1] = { ISS_ON };
            ISNewSwitch(getDeviceName(), iopin[numiopin].defVectorName, states, names, 1);
        }
    }
}
